#include "render.h"

SDL_Renderer * RENDER = NULL;
TTF_Font * FONT = NULL;

SDL_Color BG = DEF_BG_COLOR;

int yep;

/* sets rendering color */
void Render_SetColor(SDL_Color* C) {
	if (!C) return;
	SDL_SetRenderDrawColor(RENDER, C->r, C->g, C->b, C->a);
}

/* clears screen to BG */
void Render_Clear() {
	Render_SetColor(&BG);
	SDL_RenderClear(RENDER);
}

/* updates frame */
void Render_Update() {
	SDL_RenderPresent(RENDER);
}

/* loads and frees default font */
void Render_LoadFont() {
	FONT = TTF_OpenFont(DEF_FONT, 12);
}
void Render_FreeFont() {
	if (FONT) TTF_CloseFont(FONT);
}

/* renders aliased colored line */
void Render_Line(int x1, int y1, int x2, int y2, SDL_Color* C) {
	Render_SetColor(C);
	SDL_RenderDrawLine(RENDER, x1,y1, x2,y2);
}

/* renders colored rectangle */
void Render_Rect(int x, int y, int w, int h, SDL_Color* C) {
	Render_SetColor(C);
	SDL_Rect r = {x,y,w,h};
	SDL_RenderFillRect(RENDER, &r);
}

void Render_RectLine(int x, int y, int w, int h, SDL_Color* C) {
	Render_SetColor(C);
	SDL_Rect r = {x,y,w,h};
	SDL_RenderDrawRect(RENDER, &r);
}

void Render_Text(const char* text, int x, int y, int align, SDL_Color* c) {
	SDL_Color W = {0xff,0xff,0xff,0xff};
	if (!c) c = &W;

	SDL_Surface* s = TTF_RenderUTF8_Blended(FONT, text, *c);
	SDL_Texture* t = SDL_CreateTextureFromSurface(RENDER, s);

	int w,h;
	TTF_SizeText(FONT, text, &w, &h);

	SDL_Rect r = {x,y,w,h};

	if (align == ALIGN_RIGHT)
		r.x -= w;
	else if (align == ALIGN_MIDDLE)
		r.x -= w/2;

	SDL_RenderCopy(RENDER, t, NULL, &r);

	SDL_FreeSurface(s);
	SDL_DestroyTexture(t);
}

/* renders texture by texture name */
void Render_Texture(char * tname,
	SDL_Rect* src, SDL_Rect* dest, SDL_RendererFlip flip)
{
	texture * t = Texture_Get(tname);	
	if (!t) return;

	SDL_RenderCopyEx(RENDER, t->img,
	  src, dest, 0.0, NULL, flip);
}

void Render_Component(component* c) {
	if (!c) return;

	// render nodes
	SDL_Color color    = {0xff,0xff,0xff,0xff},
	          on_color = DEF_ON_COLOR;

	int x,y,w,h;
	int i;
	for (i = 0; i < c->in_count; ++i) {
		Component_GetNodeRect(c, c->in+i, &x,&y,&w,&h);
		Render_Rect(x,y,w,h, c->in[i].state ? &on_color : &color);
	}

	for (i = 0; i < c->out_count; ++i) {
		Component_GetNodeRect(c, c->out+i, &x,&y,&w,&h);
		Render_Rect(x,y,w,h, c->out[i].state ? &on_color : &color);
	}

	// render component
	if (c->render) {
		c->render(c);
		return;
	} else {
		SDL_Rect rect = {c->x, c->y, c->w, c->h};

		if (!c->state && c->img_off)
			Render_Texture(c->img_off,NULL,&rect,SDL_FLIP_NONE);
		if (c->state && c->img_on)
			Render_Texture(c->img_on,NULL,&rect,SDL_FLIP_NONE);
	}
}

void Render_Wire(wire* w) {
	if (!w) return;

	SDL_Color C = w->state ? ON_COLOR : OFF_COLOR;

	if (w->parity) {
		int xm = (w->x1 + w->x2)/2;

		Render_Line(w->x1, w->y1, xm   , w->y1, &C); // --
		Render_Line(xm   , w->y2, w->x2, w->y2, &C); // --

		Render_Line(xm   , w->y1, xm   , w->y2, &C); // |
	} else {
		int ym = (w->y1 + w->y2)/2;

		Render_Line(w->x1, w->y1, w->x1, ym   , &C); // |
		Render_Line(w->x2, ym   , w->x2, w->y2, &C); // |

		Render_Line(w->x1, ym   , w->x2, ym   , &C); // --
	}
}