#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <keyboard.h>

char **families;
char **fonts;
char *curfamily;
char *curfont;
Rectangle rfamily;
Rectangle rfont;

char* lines[] = {
	"A B C D E F G H I",
	"J K L M N O P Q R",
	"S T U V W X Y Z",
	"a b c d e f g h i",
	"j k l m n o p q r",
	"s t u v w x y z",
	"0123456789"
};

char**
familynames(void)
{
	char **f;
	Dir *d;
	int fd, n, i, j;

	fd = open("/lib/font/bit", OREAD);
	if(fd<0)
		sysfatal("open: %r");
	n = dirreadall(fd, &d);
	f = calloc(n+1, sizeof(char*));
	j = 0;
	for(i = 0; i < n; i++){
		if(d[i].qid.type & QTDIR)
			f[j++] = strdup(d[i].name);
	}
	f[j] = nil;
	free(d);
	close(fd);
	return f;
}

int
isfont(char *name)
{
	int n;

	n = strlen(name);
	return strcmp(name+n-5, ".font")==0;
}

char**
fontnames(char *family)
{
	char **names;
	Dir *dir;
	int fd, n, i, j;
	char *d;

	d = smprint("/lib/font/bit/%s", family);
	if(d==nil)
		sysfatal("smprint: %r");
	fd = open(d, OREAD);
	free(d);
	if(fd < 0)
		sysfatal("open: %r");
	n = dirreadall(fd, &dir);
	names = calloc(n+1, sizeof(char*));
	j = 0;
	for(i = 0; i < n; i++){
		if(isfont(dir[i].name))
			names[j++] = strdup(dir[i].name);
	}
	names[j] = nil;
	free(dir);
	close(fd);
	return names;
}

void
roundedborder(Image *dst, Rectangle r, int thick, Image *src, Point sp)
{
	int x0, x1, y0, y1, radius;
	Point tl, bl, tr, br;

	radius = 3;
	x0 = r.min.x;
	x1 = r.max.x-1;
	y0 = r.min.y;
	y1 = r.max.y-1;
	tl = Pt(x0+radius, y0+radius);
	bl = Pt(x0+radius, y1-radius);
	br = Pt(x1-radius, y1-radius);
	tr = Pt(x1-radius, y0+radius);
	arc(dst, tl, radius, radius, thick, src, sp, 90, 90);
	arc(dst, bl, radius, radius, thick, src, sp, 180, 90);
	arc(dst, br, radius, radius, thick, src, sp, 270, 90);
	arc(dst, tr, radius, radius, thick, src, sp, 0, 90);
	line(dst, Pt(x0, y0+radius), Pt(x0, y1-radius), 0, 0, thick, src, sp);
	line(dst, Pt(x0+radius, y1), Pt(x1-radius, y1), 0, 0, thick, src, sp);
	line(dst, Pt(x1, y1-radius), Pt(x1, y0+radius), 0, 0, thick, src, sp);
	line(dst, Pt(x1-radius, y0), Pt(x0+radius, y0), 0, 0, thick, src, sp);
}

void
redraw(void)
{
	Point p;
	Font *f;
	int i;

	p = addpt(screen->r.min, Pt(10, 10));
	draw(screen, screen->r, display->white, nil, ZP);
	string(screen, p, display->black, ZP, font, "Font Family:");
	p.y += font->height;
	rfamily = Rect(p.x, p.y, screen->r.max.x - 10, p.y + font->height + 10);
	roundedborder(screen, rfamily, 0, display->black, ZP);
	if(curfamily!=nil)
		string(screen, addpt(rfamily.min, Pt(5, 5)), display->black, ZP, font, curfamily);
	p.y = rfamily.max.y + 10;
	string(screen, p, display->black, ZP, font, "Font File:");
	p.y += font->height;
	rfont = Rect(p.x, p.y, screen->r.max.x - 10, p.y + font->height + 10);
	roundedborder(screen, rfont, 0, display->black, ZP);
	if(curfont!=nil){
		string(screen, addpt(rfont.min, Pt(5, 5)), display->black, ZP, font, curfont);
		f = openfont(display, smprint("/lib/font/bit/%s/%s", curfamily, curfont));
		p.y = rfont.max.y + 20;
		string(screen, p, display->black, ZP, f, "The quick brown fow jumps over the lazy dog");
		p.y += f->height + 20;
		for(i = 0; i < 7; i++){
			string(screen, p, display->black, ZP, f, lines[i]);
			p.y += f->height;
		}	
		freefont(f);
	}
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone)<0)
		sysfatal("cannot reattach: %r");
	redraw();
}

void
main(void)
{
	Event e;
	Menu m;
	int n;

	if(initdraw(nil, nil, "fontviewer")<0)
		sysfatal("initdraw: %r");
	einit(Emouse|Ekeyboard);
	eresized(0);
	for(;;){
		switch(event(&e)){
		case Ekeyboard:
			if(e.kbdc=='q' || e.kbdc==Kdel)
				exits(nil);
			break;
		case Emouse:
			if(e.mouse.buttons&1){
				if(ptinrect(e.mouse.xy, rfamily)){
					if(families!=nil)
						free(families);
					families = familynames();
					m.item = families;
					n = emenuhit(1, &e.mouse, &m);
					if(n>=0){
						curfamily = families[n];
						if(curfont != nil){
							curfont=nil;
						}
						redraw();
					}
				}else if(ptinrect(e.mouse.xy, rfont)){
					if(curfamily!=nil){
						if(fonts!=nil)
							free(fonts);
						fonts = fontnames(curfamily);
						m.item = fonts;
						n = emenuhit(1, &e.mouse, &m);
						if(n>=0){
							curfont = fonts[n];
							redraw();
						}
					}
				}
			}
			break;
		}
	}
}
