#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

/* 
Integrantes:

Enrico Cuono Alves Pereira - 10402875
Gabriel Mason Guerino - 10409928
Eduardo Honorio Friaça - 10408959
*/

#define BINS_HIST   256
#define LARG_HUD    420
#define ALT_HUD     520

// Botão (na janela HUD)
#define BTN_X 20
#define BTN_Y (ALT_HUD - 80)
#define BTN_W 160
#define BTN_H 44

// Cores
static const SDL_Color COR_AZUL_NEUTRO = { 30,144,255,255 };
static const SDL_Color COR_AZUL_HOVER  = {100,180,255,255 };
static const SDL_Color COR_AZUL_PRESS  = {  0, 70,160,255 };
static const SDL_Color COR_BG_HUD      = { 28, 28, 28,255 };
static const SDL_Color COR_TEXTO       = {235,235,235,255 };
static const SDL_Color COR_BARRA       = {200,200,200,255 };
static const SDL_Color COR_VERDE       = {100,200,100,255 };
static const SDL_Color COR_AMARELO     = {230,200, 80,255 };
static const SDL_Color COR_VERMELHO    = {220,100,100,255 };

typedef struct {
    SDL_Surface *origem_gs;    // imagem base (escala de cinza)
    SDL_Surface *equalizada;   // imagem equalizada
    SDL_Surface *atual;        // imagem exibida no momento
    int          esta_equalizado;
} EstadoImagem;

typedef struct {
    TTF_Font *fonte;
    int       ttf_ok; // 1=tem fonte carregada; 0=fallback sem TTF
} EstadoTexto;

/* --------- Assinaturas ---------- */
SDL_Surface *carregar_imagem(const char *caminho);
SDL_Surface *converter_para_cinza(SDL_Surface *src);
int          detectar_grayscale(SDL_Surface *s);

void         calcular_histograma(SDL_Surface *gs, int hist[BINS_HIST]);
void         equalizar_histograma(SDL_Surface *src, SDL_Surface **out);
void         calc_media_desvio(const int hist[BINS_HIST], double *media, double *desvio);

int           tentar_carregar_fonte(EstadoTexto *et, const char *arg_font);
SDL_Texture  *renderizar_texto(SDL_Renderer *r, TTF_Font *fonte, const char *txt, SDL_Color cor);
void          desenhar_hud(SDL_Renderer *r, EstadoTexto *et, int hist[BINS_HIST],
                           double media, double desvio, int equalizado, int estado_btn);
void          desenhar_botao(SDL_Renderer *r, EstadoTexto *et, int x, int y, int w, int h,
                             const SDL_Color *cor, const char *rotulo);
void          desenhar_7seg_num(SDL_Renderer *r, int x, int y, double valor, int escala);

void salvar_png_ou_bmp(SDL_Surface *s, const char *nome_png);

/* =============== MAIN =============== */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s caminho_da_imagem.ext [caminho_da_fonte.ttf]\n", argv[0]);
        return 1;
    }
    const char *caminho_img   = argv[1];
    const char *caminho_fonte = (argc >= 3) ? argv[2] : NULL;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init falhou: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }


    EstadoTexto et = {0};
    et.ttf_ok = tentar_carregar_fonte(&et, caminho_fonte);

    SDL_Surface *carregada = carregar_imagem(caminho_img);
    if (!carregada) { SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1; }

    // Detecta e, se necessário, converte para cinza (sempre padroniza Y=0.2125R+0.7154G+0.0721B)
    int ja_cinza = detectar_grayscale(carregada);
    SDL_Surface *gs = converter_para_cinza(carregada);
    SDL_DestroySurface(carregada);
    if (!gs) { fprintf(stderr, "Falha na conversão p/ cinza\n");
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Surface *eq = NULL;
    equalizar_histograma(gs, &eq);

    EstadoImagem img = {0};
    img.origem_gs = gs;
    img.equalizada = eq;
    img.atual = img.origem_gs;
    img.esta_equalizado = 0;

    // Janela principal (tamanho = imagem), centralizada
    SDL_Window *jan_princ = SDL_CreateWindow("Proj1 - Imagem", img.atual->w, img.atual->h, 0);
    if (!jan_princ) {
        fprintf(stderr, "Janela principal falhou: %s\n", SDL_GetError());
        if (eq) SDL_DestroySurface(eq);
        SDL_DestroySurface(gs);
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1;
    }
    SDL_Renderer *ren_princ = SDL_CreateRenderer(jan_princ, NULL);
    if (!ren_princ) {
        fprintf(stderr, "Renderer principal falhou: %s\n", SDL_GetError());
        SDL_DestroyWindow(jan_princ);
        if (eq) SDL_DestroySurface(eq);
        SDL_DestroySurface(gs);
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1;
    }

    // Janela HUD (ao lado da principal). 
    int x0, y0; SDL_GetWindowPosition(jan_princ, &x0, &y0);SDL_Window *jan_hud = SDL_CreateWindow("HUD - Histograma",
                                       LARG_HUD, ALT_HUD, 0);
    if (!jan_hud) {
        fprintf(stderr, "Janela HUD falhou: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren_princ);
        SDL_DestroyWindow(jan_princ);
        if (eq) SDL_DestroySurface(eq);
        SDL_DestroySurface(gs);
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte);
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_SetWindowPosition(jan_hud, x0 + img.atual->w + 20, y0);

    if (!jan_hud) {
        fprintf(stderr, "Janela HUD falhou: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren_princ); SDL_DestroyWindow(jan_princ);
        if (eq) SDL_DestroySurface(eq);
        SDL_DestroySurface(gs);
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1;
    }
    SDL_Renderer *ren_hud = SDL_CreateRenderer(jan_hud, NULL);
    if (!ren_hud) {
        fprintf(stderr, "Renderer HUD falhou: %s\n", SDL_GetError());
        SDL_DestroyWindow(jan_hud);
        SDL_DestroyRenderer(ren_princ); SDL_DestroyWindow(jan_princ);
        if (eq) SDL_DestroySurface(eq);
        SDL_DestroySurface(gs);
        SDL_Quit(); if (et.fonte) TTF_CloseFont(et.fonte); TTF_Quit(); SDL_Quit(); return 1;
    }

    SDL_Texture *tex_img = SDL_CreateTextureFromSurface(ren_princ, img.atual);

    // Histograma + métricas
    int    hist[BINS_HIST] = {0};
    double media=0.0, desvio=0.0;
    calcular_histograma(img.atual, hist);
    calc_media_desvio(hist, &media, &desvio);

    int rodando = 1;
    int estado_btn = 0; // 0=neutro,1=hover,2=pressionado

    while (rodando) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) rodando = 0;
            if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                SDL_Window *w = SDL_GetWindowFromID(ev.window.windowID);
                if (w == jan_princ || w == jan_hud) rodando = 0;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.scancode == SDL_SCANCODE_S) {
                    salvar_png_ou_bmp(img.atual, "output_image.png");
                    printf("Salvar solicitado: output_image.png (ou .bmp se PNG indisponível)\n");
                }
            }
            // mouse na HUD (botão)
            if (ev.type == SDL_EVENT_MOUSE_MOTION ||
                ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {

                if (ev.window.windowID == SDL_GetWindowID(jan_hud)) {
                    int mx = ev.motion.x, my = ev.motion.y;
                    int dentro = (mx>=BTN_X && mx<=BTN_X+BTN_W && my>=BTN_Y && my<=BTN_Y+BTN_H);

                    if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                        estado_btn = dentro ? 1 : 0;
                    } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (dentro && ev.button.button == SDL_BUTTON_LEFT) estado_btn = 2;
                    } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                        if (ev.button.button == SDL_BUTTON_LEFT) {
                            if (estado_btn == 2 && dentro) {
                                // toggle equalização
                                if (!img.esta_equalizado) {
                                    img.atual = img.equalizada;
                                    img.esta_equalizado = 1;
                                } else {
                                    img.atual = img.origem_gs;
                                    img.esta_equalizado = 0;
                                }
                                if (tex_img) SDL_DestroyTexture(tex_img);
                                tex_img = SDL_CreateTextureFromSurface(ren_princ, img.atual);
                                calcular_histograma(img.atual, hist);
                                calc_media_desvio(hist, &media, &desvio);
                            }
                            estado_btn = dentro ? 1 : 0;
                        }
                    }
                }
            }
        }

        // desenhar principal
        SDL_SetRenderDrawColor(ren_princ, 18,18,18,255);
        SDL_RenderClear(ren_princ);
        if (tex_img) {
            SDL_FRect dst = {0,0,img.atual->w, img.atual->h};
            SDL_RenderTexture(ren_princ, tex_img, NULL, &dst);
        }
        SDL_RenderPresent(ren_princ);

        // desenhar HUD
        desenhar_hud(ren_hud, &et, hist, media, desvio, img.esta_equalizado, estado_btn);
        SDL_RenderPresent(ren_hud);

        SDL_Delay(16);
    }

    // limpeza
    if (tex_img) SDL_DestroyTexture(tex_img);
    SDL_DestroyRenderer(ren_hud); SDL_DestroyWindow(jan_hud);
    SDL_DestroyRenderer(ren_princ); SDL_DestroyWindow(jan_princ);
    if (img.equalizada) SDL_DestroySurface(img.equalizada);
    if (img.origem_gs)  SDL_DestroySurface(img.origem_gs);
    if (et.fonte) TTF_CloseFont(et.fonte);
    TTF_Quit(); SDL_Quit();
    return 0;
}

/* ============ Implementações ============ */

SDL_Surface *carregar_imagem(const char *caminho) {
    SDL_Surface *s = IMG_Load(caminho);
    if (!s) fprintf(stderr, "IMG_Load falhou: %s\n", SDL_GetError());
    return s;
}

int detectar_grayscale(SDL_Surface *s) {
    int w = s->w;
    int h = s->h;
    
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(s->format);
    int bpp = fmt->bytes_per_pixel;

    SDL_LockSurface(s);
    Uint8 *px = (Uint8*)s->pixels;
    int step_x = (w>200)?(w/100):1, step_y=(h>200)?(h/100):1;
    for (int y=0;y<h;y+=step_y) for (int x=0;x<w;x+=step_x) {
        Uint8 *p = px + y*s->pitch + x*bpp;
        Uint32 pix=0;
        switch (bpp) {
            case 1: pix = p[0]; break;
            case 2: pix = p[0]|(p[1]<<8); break;
            case 3:
            #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                pix = p[0]<<16 | p[1]<<8 | p[2];
            #else
                pix = p[0] | p[1]<<8 | p[2]<<16;
            #endif
                break;
            default: pix = p[0]|(p[1]<<8)|(p[2]<<16)|((bpp==4)?(p[3]<<24):0); break;
        }
        Uint8 r,g,b,a; 
        const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(s->format);
        int bpp = fmt->bytes_per_pixel;
        SDL_GetRGBA(pix, fmt, NULL, &r,&g,&b,&a);
        if (!(r==g && g==b)) { SDL_UnlockSurface(s); return 0; }
    }
    SDL_UnlockSurface(s);
    return 1;
}

SDL_Surface *converter_para_cinza(SDL_Surface *src) {
    int w=src->w, h=src->h;
    SDL_Surface *out = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGB24);
    if (!out) return NULL;

    SDL_LockSurface(src); SDL_LockSurface(out);
    Uint8 *ps=(Uint8*)src->pixels, *pd=(Uint8*)out->pixels;
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(src->format);
    int bpp_s = fmt->bytes_per_pixel;
    int pitch_s = src->pitch;
    int pitch_d = out->pitch;

    for (int y=0;y<h;y++) for (int x=0;x<w;x++) {
        Uint8 *sp = ps + y*pitch_s + x*bpp_s;
        Uint32 pix=0;
        switch (bpp_s) {
            case 1: pix = sp[0]; break;
            case 2: pix = sp[0]|(sp[1]<<8); break;
            case 3:
            #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                pix = sp[0]<<16 | sp[1]<<8 | sp[2];
            #else
                pix = sp[0] | sp[1]<<8 | sp[2]<<16;
            #endif
                break;
            default: pix = sp[0]|(sp[1]<<8)|(sp[2]<<16)|((bpp_s==4)?(sp[3]<<24):0); break;
        }
        const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(src->format);
        Uint8 r,g,b,a;
        SDL_GetRGBA(pix, fmt, NULL, &r, &g, &b, &a);
        double Y = 0.2125*(double)r + 0.7154*(double)g + 0.0721*(double)b;
        if (Y<0) Y=0; if (Y>255) Y=255;
        Uint8 yv = (Uint8)(Y+0.5);
        Uint8 *dp = pd + y*pitch_d + x*3;
        dp[0]=yv; dp[1]=yv; dp[2]=yv;
    }

    SDL_UnlockSurface(out); SDL_UnlockSurface(src);
    return out;
}

void calcular_histograma(SDL_Surface *gs, int hist[BINS_HIST]) {
    for (int i=0;i<BINS_HIST;i++) hist[i]=0;
    SDL_LockSurface(gs);
    int w = gs->w;
    int h = gs->h;

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(gs->format);
    int bpp = fmt->bytes_per_pixel;
    int pitch = gs->pitch;

    Uint8 *px=(Uint8*)gs->pixels;
    for (int y=0;y<h;y++) for (int x=0;x<w;x++) {
        Uint8 *p = px + y*pitch + x*bpp;
        Uint8 v = p[0]; // RGB24: R==G==B
        hist[v]++;
    }
    SDL_UnlockSurface(gs);
}

void equalizar_histograma(SDL_Surface *src, SDL_Surface **out) {
    int hist[BINS_HIST]; calcular_histograma(src, hist);
    int total = src->w * src->h;
    double cdf[BINS_HIST]; int acc=0;
    for (int i=0;i<BINS_HIST;i++){ acc+=hist[i]; cdf[i]=(double)acc/(double)total; }
    
    SDL_Surface *dest = SDL_CreateSurface(src->w, src->h, SDL_PIXELFORMAT_RGB24);

    if (!dest){ *out=NULL; return; }

    SDL_LockSurface(src); SDL_LockSurface(dest);
    Uint8 *ps=(Uint8*)src->pixels, *pd=(Uint8*)dest->pixels;
    int w = src->w;
    int h = src->h;

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(src->format);
    int bpp = fmt->bytes_per_pixel;

    int pitch_s = src->pitch;
    int pitch_d = dest->pitch;


    for (int y=0;y<h;y++) for (int x=0;x<w;x++) {
        Uint8 *sp = ps + y*pitch_s + x*bpp;
        Uint8 v = sp[0];
        Uint8 m = (Uint8)floor(cdf[v]*255.0 + 0.5);
        Uint8 *dp = pd + y*pitch_d + x*3;
        dp[0]=m; dp[1]=m; dp[2]=m;
    }

    SDL_UnlockSurface(dest); SDL_UnlockSurface(src);
    *out = dest;
}

void calc_media_desvio(const int hist[BINS_HIST], double *media, double *desvio) {
    long total=0; double soma=0.0;
    for (int i=0;i<BINS_HIST;i++){ total+=hist[i]; soma += (double)i * (double)hist[i]; }
    if (total==0){ *media=0; *desvio=0; return; }
    *media = soma / (double)total;
    double var=0.0;
    for (int i=0;i<BINS_HIST;i++){
        double d = (double)i - *media;
        var += d*d * (double)hist[i];
    }
    var /= (double)total;
    *desvio = sqrt(var);
}

/* ---------- Texto (SDL_ttf) ---------- */
int tentar_carregar_fonte(EstadoTexto *et, const char *arg_font) {
    const char *candidatos[] = {
        arg_font,
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/Library/Fonts/Arial.ttf",
        "C:\\\\Windows\\\\Fonts\\\\arial.ttf",
        NULL
    };

    for (int i=0; candidatos[i]; i++) {
        if (!candidatos[i]) continue;

        et->fonte = TTF_OpenFont(candidatos[i], 18);
        if (et->fonte) {
            fprintf(stderr, "Fonte carregada: %s\n", candidatos[i]);
            return 1;
        } else {
            fprintf(stderr, "Falha ao abrir fonte %s: %s\n", candidatos[i], SDL_GetError());
        }
    }

    fprintf(stderr, "Aviso: nenhuma fonte TTF encontrada. Seguindo sem texto TTF.\n");
    return 0;
}


SDL_Texture *renderizar_texto(SDL_Renderer *r, TTF_Font *fonte, const char *txt, SDL_Color cor) {
    if (!fonte || !txt) return NULL;
    SDL_Surface *surf = TTF_RenderText_Solid(fonte, txt, strlen(txt), cor);
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_DestroySurface(surf);
    return tex;
}

/* ---------- HUD ---------- */
static void fill(SDL_Renderer *r, int x,int y,int w,int h, SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_FRect rc={x,y,w,h};
    SDL_RenderFillRect(r,&rc);
}

void desenhar_botao(SDL_Renderer *r, EstadoTexto *et, int x,int y,int w,int h,
                    const SDL_Color *cor, const char *rotulo) {
    // borda
    fill(r,x,y,w,h,*cor);
    // miolo
    fill(r,x+4,y+4,w-8,h-8,COR_BG_HUD);
    // rótulo
    if (et->ttf_ok) {
        SDL_Texture *t = renderizar_texto(r, et->fonte, rotulo, *cor);
        if (t){
            float tw, th;
            SDL_GetTextureSize(t, &tw, &th);
            SDL_FRect dst = { x + (w - tw)/2.0f, y + (h - th)/2.0f, tw, th };
            SDL_RenderTexture(r, t, NULL, &dst);
            SDL_DestroyTexture(t);
        }
    }
}

void desenhar_7seg_num(SDL_Renderer *r, int x,int y,double valor,int escala){
    // fallback numérico quando não há TTF
    int inteiro=(int)floor(valor+0.5);
    char buf[32]; snprintf(buf,sizeof(buf), "%d", inteiro);
    SDL_Color c = COR_TEXTO;
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    // Caixinhas simples: uma barra por dígito (indicativo)
    int i=0; while(buf[i] && i<10){
        fill(r, x+i*(10*escala), y, 8*escala, 18*escala, c);
        i++;
    }
}

void desenhar_hud(SDL_Renderer *r, EstadoTexto *et, int hist[BINS_HIST],
                  double media, double desvio, int equalizado, int estado_btn) {

    SDL_SetRenderDrawColor(r, COR_BG_HUD.r,COR_BG_HUD.g,COR_BG_HUD.b,COR_BG_HUD.a);
    SDL_RenderClear(r);

    int pad=12;
    int hw=LARG_HUD-2*pad, hh=320;
    int hx=pad, hy=pad;

    // fundo histograma
    fill(r, hx, hy, hw, hh, (SDL_Color){40,40,40,255});

    // barras
    int maxv=1; for(int i=0;i<BINS_HIST;i++) if(hist[i]>maxv) maxv=hist[i];
    int bw = hw / BINS_HIST; if (bw<1) bw=1;
    for (int i=0;i<BINS_HIST;i++) {
        double rel = (double)hist[i]/(double)maxv;
        int bh = (int)floor(rel*(hh-20));
        SDL_Color c = COR_BARRA;
        fill(r, hx + i*bw, hy + hh - bh - 2, bw, bh, c);
    }

    // textos (média/desvio + classificações)
    int tx = hx, ty = hy + hh + 10;
    char buf_media[64], buf_desvio[64];
    snprintf(buf_media, sizeof(buf_media), "Media (0-255): %.1f", media);
    snprintf(buf_desvio, sizeof(buf_desvio), "Desvio padrao: %.1f", desvio);

    const char *classe_media = (media>=170.0)? "clara" : (media>=85.0)? "media" : "escura";
    const char *classe_contr = (desvio>=60.0)? "alto" : (desvio>=25.0)? "medio" : "baixo";
    SDL_Color cor_media = (media>=170.0)? COR_VERDE : (media>=85.0)? COR_AMARELO : COR_VERMELHO;
    SDL_Color cor_contr = (desvio>=60.0)? COR_VERDE : (desvio>=25.0)? COR_AMARELO : COR_VERMELHO;

    if (et->ttf_ok) {
        SDL_Texture *t1 = renderizar_texto(r, et->fonte, buf_media, COR_TEXTO);
        SDL_Texture *t2 = renderizar_texto(r, et->fonte, buf_desvio, COR_TEXTO);
        SDL_Texture *t3 = renderizar_texto(r, et->fonte, "Intensidade:", COR_TEXTO);
        SDL_Texture *t4 = renderizar_texto(r, et->fonte, "Contraste:",   COR_TEXTO);
        SDL_Texture *t5 = renderizar_texto(r, et->fonte, classe_media, cor_media);
        SDL_Texture *t6 = renderizar_texto(r, et->fonte, classe_contr, cor_contr);

        float tw,th; SDL_FRect dst;

        if (t1){ SDL_GetTextureSize(t1,&tw,&th); dst=(SDL_FRect){tx,ty,tw,th}; SDL_RenderTexture(r,t1,NULL,&dst); ty+=th+6; }
        if (t2){ SDL_GetTextureSize(t2,&tw,&th); dst=(SDL_FRect){tx,ty,tw,th}; SDL_RenderTexture(r,t2,NULL,&dst); ty+=th+10; }

        if (t3){ SDL_GetTextureSize(t3,&tw,&th); dst=(SDL_FRect){tx,ty,tw,th}; SDL_RenderTexture(r,t3,NULL,&dst); }
        if (t5){ SDL_GetTextureSize(t5,&tw,&th); dst=(SDL_FRect){tx+140,ty,tw,th}; SDL_RenderTexture(r,t5,NULL,&dst); }
        ty +=  th + 6;

        if (t4){ SDL_GetTextureSize(t4,&tw,&th); dst=(SDL_FRect){tx,ty,tw,th}; SDL_RenderTexture(r,t4,NULL,&dst); }
        if (t6){ SDL_GetTextureSize(t6,&tw,&th); dst=(SDL_FRect){tx+140,ty,tw,th}; SDL_RenderTexture(r,t6,NULL,&dst); }

        if (t1) SDL_DestroyTexture(t1);
        if (t2) SDL_DestroyTexture(t2);
        if (t3) SDL_DestroyTexture(t3);
        if (t4) SDL_DestroyTexture(t4);
        if (t5) SDL_DestroyTexture(t5);
        if (t6) SDL_DestroyTexture(t6);
    } else {
        // Sem TTF: mostra números “indicativos”
        desenhar_7seg_num(r, tx, ty, media, 2);  ty += 30;
        desenhar_7seg_num(r, tx, ty, desvio, 2); ty += 30;
        // Caixas de classe
        fill(r, tx, ty, 120, 20, cor_media); ty += 26;
        fill(r, tx, ty, 120, 20, cor_contr); ty += 26;
    }

    // Botão
    const SDL_Color *cbtn = &COR_AZUL_NEUTRO;
    if (estado_btn==1) cbtn=&COR_AZUL_HOVER;
    if (estado_btn==2) cbtn=&COR_AZUL_PRESS;

    desenhar_botao(r, et, BTN_X, BTN_Y, BTN_W, BTN_H, cbtn,
                   equalizado ? "Original" : "Equalizar");

    // Dica de salvar
    if (et->ttf_ok) {
        SDL_Texture *d = renderizar_texto(r, et->fonte, "Tecla S: salvar output_image.png", COR_TEXTO);
        if (d){
           float tw, th;
            SDL_GetTextureSize(d, &tw, &th);
            SDL_FRect dst = { BTN_X + BTN_W + 16, BTN_Y + (BTN_H - th)/2.0f, tw, th };
            SDL_RenderTexture(r, d, NULL, &dst);
            SDL_DestroyTexture(d);
        }
    }
}

/* ---------- Salvar PNG (fallback BMP) ---------- */
void salvar_png_ou_bmp(SDL_Surface *s, const char *nome_png) {
#ifdef IMG_SavePNG
    if (IMG_SavePNG(s, nome_png) == 0) {
        printf("Salvo: %s\n", nome_png);
        return;
    } else {
        fprintf(stderr, "IMG_SavePNG falhou, tentando BMP...\n");
    }
#endif
    char nome_bmp[256];
    strncpy(nome_bmp, nome_png, sizeof(nome_bmp)-1); nome_bmp[sizeof(nome_bmp)-1]=0;
    char *p = strrchr(nome_bmp,'.'); if (p) *p=0;
    strcat(nome_bmp, ".bmp");
    if (SDL_SaveBMP(s, nome_bmp) == 0) printf("Salvo (BMP): %s\n", nome_bmp);
    else fprintf(stderr, "Falha ao salvar BMP: %s\n", SDL_GetError());
}
