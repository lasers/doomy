/*
Copyright 10/30/2026 08/02/2024 https://github.com/su8/doomy
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.

Compile with:
gcc -Wall -Wextra -O2 -I/usr/include/freetype2 -lX11 -lXft -lXrender -o doomy doomy.c
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrender.h>

/* How the background of the bar is made */
#define TRANSPARENT_NONE   0U /* solid, as it always was */
#define TRANSPARENT_ARGB   1U /* a 32 bit window with an alpha channel, a compositor (picom, xcompmgr ...) blends it */
#define TRANSPARENT_PSEUDO 2U /* the wallpaper that is behind the bar, tinted: needs no compositor */
#define VLA 1000

static inline void drawString(const char *);
static int hasCompositor(Display *, int);
static unsigned int pickTransparency(Display *, int, XVisualInfo *);
static void drawBackground(Display *, Window, Visual *, unsigned int, unsigned int);
static error_t parse_opt(int , char *, struct argp_state *);

static XftColor brown;
static XftColor pink;
static XftColor redColor;
static XftColor greenColor;
static XftColor blueColor;
static XftColor yellowColor;
static XftFont *xftfont;
static XftDraw *xdraw = NULL;
static const char *BG_COLOR = "";
static const char *TEXT_COLOR = "";
static const char *USE_FONT = "";
static const char *TMODE = "";
static unsigned int BAR_HEIGHT = 18U;
static unsigned int TOP_BAR = 1U;
static unsigned int ALPHA = 100U;
static unsigned int tmode = TRANSPARENT_NONE;

static char doc[] = "Dead simple x11 bar/statusline program.\v";

const char *argp_program_version = "1.0";
const char *argp_program_bug_address = "https://github.com/su8/doomy ";

static struct argp_option options[] =
{
  { .doc = "" },
  { .name = "bgcolor", .key = 'b', .arg = "#282a2e", .doc = "Set background color"},
  { .name = "textcolor", .key = 't', .arg = "#b294bb", .doc = "Set text color"},
  { .name = "top", .key = 's', .arg = "1", .doc = "Show the program on top == 1, else 0 bottom position"},
  { .name = "font", .key = 'f', .arg = "xft#DejaVu Sans:size=9:style=bold", .doc = "Use custom font size and bold type"},
  { .name = "barheight", .key = 'h', .arg = "15", .doc = "Set the height of the bar"},
  { .name = "alpha", .key = 'a', .arg = "70", .doc = "Opacity of the background in percent, 100 is solid (the default), 0 is invisible. The text stays solid"},
  { .name = "tmode", .key = 'm', .arg = "auto", .doc = "How to be transparent: argb (needs a compositor like picom or xcompmgr), pseudo (shows the wallpaper through the bar, no compositor needed), auto picks argb when a compositor runs, else pseudo"},
  { .doc = NULL }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  switch(key)
  {
    case 'b':
      BG_COLOR = arg;
      break;
    case 't':
      TEXT_COLOR = arg;
      break;
    case 'f':
      USE_FONT = arg;
      break;
    case 'h':
      BAR_HEIGHT = (unsigned int)strtoul(arg, (char **)NULL, 10);
      break;
    case 's':
      TOP_BAR = (unsigned int)strtoul(arg, (char **)NULL, 10);
      break;
    case 'a':
      ALPHA = (unsigned int)strtoul(arg, (char **)NULL, 10);
      if (ALPHA > 100U)
      {
        ALPHA = 100U;
      }
      break;
    case 'm':
      if (strcmp(arg, "auto") && strcmp(arg, "argb") && strcmp(arg, "pseudo"))
      {
        argp_error(state, "--tmode must be auto, argb or pseudo, not '%s'", arg);
      }
      TMODE = arg;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return EXIT_SUCCESS;
}

static const struct argp arg_parser = {
  .parser = parse_opt,
  .options = options,
  .doc = doc
};

int main(int argc, char *argv[])
{
  Window win;
  Display *display;
  Screen *Screen;
  int screen;
  int depth;
  XSetWindowAttributes wa;
  XVisualInfo vinfo;
  unsigned long valuemask;
  Colormap cmap;
  Colormap argbColormap = 0;
  Visual *visual;
  FILE *fp = NULL;
  unsigned int showToBottom = 0U;
  char buf[VLA] = {'\0'};
  const char *use_font = *USE_FONT ? USE_FONT : "xft#DejaVu Sans:size=9:style=bold";

  argp_parse(&arg_parser, argc, argv, ARGP_IN_ORDER, NULL, NULL);

  display = XOpenDisplay(NULL);
  if (NULL == display)
  {
    puts("Cannot open X display.");
    return EXIT_FAILURE;
  }

  screen = DefaultScreen(display);
  cmap = DefaultColormap(display, screen);
  visual = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);
  Screen = ScreenOfDisplay(display, DefaultScreen(display));
  showToBottom = (unsigned int)Screen->height - (unsigned int)BAR_HEIGHT;
  tmode = pickTransparency(display, screen, &vinfo);

  wa.override_redirect = 1;
  wa.event_mask = ExposureMask|KeyPressMask;
  valuemask = CWOverrideRedirect | CWEventMask;
  if (TRANSPARENT_ARGB == tmode)
  {
    /* a window with an alpha channel needs its own visual and colormap, and no parent relative background */
    visual = vinfo.visual;
    depth = 32;
    cmap = XCreateColormap(display, RootWindow(display, screen), visual, AllocNone);
    argbColormap = cmap;
    wa.colormap = cmap;
    wa.border_pixel = 0;
    wa.background_pixel = 0;
    valuemask |= CWColormap | CWBorderPixel | CWBackPixel;
  }
  else
  {
    wa.background_pixmap = ParentRelative;
    valuemask |= CWBackPixmap;
  }

  win = XCreateWindow(display, RootWindow(display, screen),
    0, TOP_BAR ? 0 : showToBottom,
    (unsigned int)Screen->width, BAR_HEIGHT ? BAR_HEIGHT : 15U, 0,
    depth, (unsigned int)(TRANSPARENT_ARGB == tmode ? InputOutput : CopyFromParent), visual,
    valuemask, &wa
  );

  XSelectInput(display, win, StructureNotifyMask);
  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(display, win, &wm_delete_window, 1);

  XftColorAllocName(display, visual, cmap, *BG_COLOR ? BG_COLOR : "#282a2e", &brown);
  XftColorAllocName(display, visual, cmap, *TEXT_COLOR ? TEXT_COLOR : "#b294bb", &pink);
  XftColorAllocName(display, visual, cmap, "#ff5555", &redColor);
  XftColorAllocName(display, visual, cmap, "#50fa7b", &greenColor);
  XftColorAllocName(display, visual, cmap, "#8be9fd", &blueColor);
  XftColorAllocName(display, visual, cmap, "#f1fa8c", &yellowColor);
  xftfont = XftFontOpenXlfd(display, screen, use_font);
  if (!xftfont)
  {
    xftfont = XftFontOpenName(display, screen, use_font);
    if (!xftfont)
    {
      return EXIT_FAILURE;
    }
  }

  XMapWindow(display, win);
  XFlush(display);

  while (1)
  {
    if (!(fp = fopen("/tmp/doomy.txt", "r")))
    {
      puts("Couldn't find /tmp/doomy.txt, exiting.");
      goto err;
    }
    fscanf(fp, "%[^\n]\n", buf);
    if ((fclose(fp)) == EOF)
    {
      puts("EOF in fclose(fp), exiting.");
      goto err;
    }
    xdraw = XftDrawCreate(display, win, visual, cmap);
    drawBackground(display, win, visual, (unsigned int)Screen->width, BAR_HEIGHT ? BAR_HEIGHT : 15U);
    drawString(buf);
    XFlush(display);
    sleep(1);
    XftDrawDestroy(xdraw);
  }

err:
  XftColorFree(display, visual, cmap, &brown);
  XftColorFree(display, visual, cmap, &pink);
  XftColorFree(display, visual, cmap, &redColor);
  XftColorFree(display, visual, cmap, &greenColor);
  XftColorFree(display, visual, cmap, &blueColor);
  XftColorFree(display, visual, cmap, &yellowColor);
  XftFontClose(display, xftfont);
  if (xdraw != NULL)
  {
    XftDrawDestroy(xdraw);
  }
  if (display != NULL)
  {
    XDestroyWindow(display, win);
    if (argbColormap)
    {
      XFreeColormap(display, argbColormap);
    }
    XCloseDisplay(display);
  }
  return EXIT_SUCCESS;
}

/* A compositing manager (picom, xcompmgr, compton ...) owns the _NET_WM_CM_S<screen> selection */
static int hasCompositor(Display *display, int screenNumber)
{
  char name[32] = {'\0'};
  snprintf(name, sizeof(name) - 1, "_NET_WM_CM_S%d", screenNumber);
  return XGetSelectionOwner(display, XInternAtom(display, name, False)) != None;
}

/* Which of the three kinds of background to use. vinfo gets the 32 bit visual for ARGB */
static unsigned int pickTransparency(Display *display, int screenNumber, XVisualInfo *vinfo)
{
  const char *want = *TMODE ? TMODE : "auto";
  int compositor = 0;
  XRenderPictFormat *format = NULL;

  if (ALPHA >= 100U)
  {
    return TRANSPARENT_NONE;
  }
  compositor = hasCompositor(display, screenNumber);
  if (strcmp(want, "pseudo") != 0 && (strcmp(want, "argb") == 0 || compositor))
  {
    if (XMatchVisualInfo(display, screenNumber, 32, TrueColor, vinfo)
        && (format = XRenderFindVisualFormat(display, vinfo->visual)) != NULL
        && format->direct.alphaMask != 0)
    {
      if (!compositor)
      {
        fputs("doomy: no compositing manager is running, the bar will not be transparent. Start picom or xcompmgr, or use --tmode pseudo\n", stderr);
      }
      return TRANSPARENT_ARGB;
    }
    if (strcmp(want, "argb") == 0)
    {
      fputs("doomy: there is no 32 bit visual with an alpha channel, using pseudo transparency\n", stderr);
    }
  }
  return TRANSPARENT_PSEUDO;
}

/* The background of the bar. Solid is the old way. For the transparent ones the colour is drawn with XRender,
 * premultiplied by the alpha: ARGB replaces the pixels (Src), pseudo blends the colour over the wallpaper (Over) */
static void drawBackground(Display *display, Window win, Visual *visual, unsigned int width, unsigned int height)
{
  XRenderColor color;
  XRenderPictFormat *format = NULL;
  Picture picture;

  if (TRANSPARENT_NONE == tmode || (format = XRenderFindVisualFormat(display, visual)) == NULL)
  {
    XftDrawRect(xdraw, &brown, 0, 0, width, height);
    return;
  }
  color.red = (unsigned short)((unsigned int)brown.color.red * ALPHA / 100U);
  color.green = (unsigned short)((unsigned int)brown.color.green * ALPHA / 100U);
  color.blue = (unsigned short)((unsigned int)brown.color.blue * ALPHA / 100U);
  color.alpha = (unsigned short)(65535U * ALPHA / 100U);
  if (TRANSPARENT_PSEUDO == tmode)
  {
    XClearWindow(display, win); /* the parent relative background: the wallpaper that is behind the bar */
  }
  picture = XRenderCreatePicture(display, win, format, 0, NULL);
  XRenderFillRectangle(display, TRANSPARENT_ARGB == tmode ? PictOpSrc : PictOpOver, picture, &color, 0, 0, width, height);
  XRenderFreePicture(display, picture);
}

static inline void drawString(const char *str)
{
  int x = 0;
  XftColor *current = &pink;
  XGlyphInfo extents;
  while (*str) {
    if (!strncmp(str, "[red]", 5))
    {
      current = &redColor;
      str += 5;
      continue;
    }
    if (!strncmp(str, "[green]", 7))
    {
      current = &greenColor;
      str += 7;
      continue;
    }
    if (!strncmp(str, "[blue]", 6))
    {
      current = &blueColor;
      str += 6;
      continue;
    }
    if (!strncmp(str, "[yellow]", 8))
    {
      current = &yellowColor;
      str += 8;
      continue;
    }
    if (!strncmp(str, "[/]", 3))
    {
      current = &pink;
      str += 3;
      continue;
    }
    char segment[VLA] = {'\0'};
    int i = 0;
    while (*str &&
               strncmp(str, "[red]", 5) &&
               strncmp(str, "[green]", 7) &&
               strncmp(str, "[blue]", 6) &&
               strncmp(str, "[yellow]", 8) &&
               strncmp(str, "[/]", 3))
    {
      segment[i++] = *str++;
      if (i == VLA - 1) break;
    }
    segment[i] = '\0';
    XftTextExtentsUtf8(XftDrawDisplay(xdraw), xftfont, (FcChar8 *)segment, strlen(segment), &extents);
    XftDrawStringUtf8(xdraw, current, xftfont, x, 1 + xftfont->ascent, (FcChar8 *)segment, strlen(segment));
    x += extents.xOff;
  }
}