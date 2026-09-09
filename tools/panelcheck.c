/* Parses each panel with the same nanosvg build Rack uses, at Rack's 75 DPI,
   and reports the page size in pixels plus the bounding box of everything that
   will actually be drawn. Inkscape is not a good check for this: it renders
   things nanosvg silently drops. */

#include <stdio.h>
#include <string.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>

int main(int argc, char** argv) {
	int bad = 0;
	for (int i = 1; i < argc; i++) {
		NSVGimage* img = nsvgParseFromFile(argv[i], "px", 75.f);
		if (!img) {
			printf("%-24s PARSE FAILED\n", argv[i]);
			bad++;
			continue;
		}
		int shapes = 0, visible = 0;
		float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
		for (NSVGshape* s = img->shapes; s; s = s->next) {
			shapes++;
			if (!(s->flags & NSVG_FLAGS_VISIBLE))
				continue;
			visible++;
			if (s->bounds[0] < x0) x0 = s->bounds[0];
			if (s->bounds[1] < y0) y0 = s->bounds[1];
			if (s->bounds[2] > x1) x1 = s->bounds[2];
			if (s->bounds[3] > y1) y1 = s->bounds[3];
		}
		float hp = img->width / 15.f;
		int hpOk = (fabsf(hp - (float)(int)(hp + 0.5f)) < 0.001f);
		int fits = (x0 > -0.01f && y0 > -0.01f && x1 < img->width + 0.01f && y1 < img->height + 0.01f);
		/* shapes must exceed visible, or the hidden components layer is being
		   drawn onto the panel. */
		printf("%-28s %8.3f x %8.3f px  %5.2f HP  shapes %3d visible %3d  bbox %.2f %.2f %.2f %.2f  %s%s\n",
			argv[i], img->width, img->height, hp, shapes, visible, x0, y0, x1, y1,
			hpOk ? "" : "HP-NOT-INTEGER ", fits ? "" : "OVERFLOW");
		if (!hpOk || !fits || shapes == visible)
			bad++;
		nsvgDelete(img);
	}
	printf("%s\n", bad ? "PROBLEMS" : "panels ok");
	return bad ? 1 : 0;
}
