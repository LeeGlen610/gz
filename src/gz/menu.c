#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <list/list.h>
#include "gfx.h"
#include "menu.h"
#include "z64.h"

void menu_init(struct menu *menu, int cell_width, int cell_height,
               struct gfx_font *font)
{
  menu->cxoffset = MENU_NOVALUE;
  menu->cyoffset = MENU_NOVALUE;
  menu->pxoffset = MENU_NOVALUE;
  menu->pyoffset = MENU_NOVALUE;
  menu->cell_width = cell_width;
  menu->cell_height = cell_height;
  menu->font = font;
  menu->alpha = 1.f;
  list_init(&menu->items, sizeof(struct menu_item));
  menu->selector = NULL;
  menu->parent = NULL;
  menu->child = NULL;
  menu->highlight_color_animated = 0x000000;
  menu->highlight_color_static = 0x5050FF;
  menu->highlight_state[0] = 17;
  menu->highlight_state[1] = 19;
  menu->highlight_state[2] = 23;
}

void menu_destroy(struct menu *menu)
{
  while (menu->items.first)
    menu_item_remove(menu->items.first);
}

int menu_get_cxoffset(struct menu *menu, _Bool inherit)
{
  int cxoffset = menu->cxoffset;
  if (inherit && menu->parent)
    cxoffset += menu_get_cxoffset(menu->parent, 1);
  return cxoffset;
}

void menu_set_cxoffset(struct menu *menu, int cxoffset)
{
  menu->cxoffset = cxoffset;
}

int menu_get_cyoffset(struct menu *menu, _Bool inherit)
{
  int cyoffset = menu->cyoffset;
  if (inherit && menu->parent)
    cyoffset += menu_get_cyoffset(menu->parent, 1);
  return cyoffset;
}

void menu_set_cyoffset(struct menu *menu, int cyoffset)
{
  menu->cyoffset = cyoffset;
}

int menu_get_pxoffset(struct menu *menu, _Bool inherit)
{
  int pxoffset = menu->pxoffset;
  if (inherit && menu->parent)
    pxoffset += menu_get_pxoffset(menu->parent, 1);
  return pxoffset;
}

void menu_set_pxoffset(struct menu *menu, int pxoffset)
{
  menu->pxoffset = pxoffset;
}

int menu_get_pyoffset(struct menu *menu, _Bool inherit)
{
  int pyoffset = menu->pyoffset;
  if (inherit && menu->parent)
    pyoffset += menu_get_pyoffset(menu->parent, 1);
  return pyoffset;
}

void menu_set_pyoffset(struct menu *menu, int pyoffset)
{
  menu->pyoffset = pyoffset;
}

int menu_get_cell_width(struct menu *menu, _Bool inherit)
{
  if (inherit && menu->parent && menu->cell_width == MENU_NOVALUE)
    return menu_get_cell_width(menu->parent, 1);
  return menu->cell_width;
}

void menu_set_cell_width(struct menu *menu, int cell_width)
{
  menu->cell_width = cell_width;
}

int menu_get_cell_height(struct menu *menu, _Bool inherit)
{
  if (inherit && menu->parent && menu->cell_height == MENU_NOVALUE)
    return menu_get_cell_height(menu->parent, 1);
  return menu->cell_height;
}

void menu_set_cell_height(struct menu *menu, int cell_height)
{
  menu->cell_height = cell_height;
}

struct gfx_font *menu_get_font(struct menu *menu, _Bool inherit)
{
  if (inherit && menu->parent && menu->font == MENU_NOVALUE)
    return menu_get_font(menu->parent, 1);
  return menu->font;
}

void menu_set_font(struct menu *menu, struct gfx_font *font)
{
  menu->font = font;
}

float menu_get_alpha(struct menu *menu, _Bool inherit)
{
  float alpha = menu->alpha;
  if (inherit && menu->parent)
    alpha *= menu_get_alpha(menu->parent, 1);
  return alpha;
}

uint8_t menu_get_alpha_i(struct menu *menu, _Bool inherit)
{
  float alpha = menu_get_alpha(menu, inherit);
  if (alpha < 0.f)
    alpha = 0.f;
  else if (alpha > 1.f)
    alpha = 1.f;
  return alpha * 0xFF;
}

void menu_set_alpha(struct menu *menu, float alpha)
{
  menu->alpha = alpha;
}

int menu_cell_screen_x(struct menu *menu, int x)
{
  return (x + menu_get_cxoffset(menu, 1)) * menu_get_cell_width(menu, 1) +
         menu_get_pxoffset(menu, 1);
}

int menu_cell_screen_y(struct menu *menu, int y)
{
  return (y + menu_get_cyoffset(menu, 1)) * menu_get_cell_height(menu, 1) +
         menu_get_pyoffset(menu, 1);
}

int menu_think(struct menu *menu)
{
  if (menu->child)
    return menu_think(menu->child);
  for (struct menu_item *item = menu->items.first;
       item; item = list_next(item))
  {
    if (item->think_proc && item->think_proc(item))
      return 1;
    if (item->imenu && menu_think(item->imenu))
      return 1;
  }
  return 0;
}

void menu_draw(struct menu *menu)
{
  if (menu->child)
    return menu_draw(menu->child);
  for (int i = 0; i < 3; ++i) {
    int shift = i * 8;
    uint32_t mask = 0xFF << shift;
    int v = (menu->highlight_color_animated & mask) >> shift;
    v += menu->highlight_state[i];
    if (v < 0x00 || v > 0xFF) {
      v = -v + (v > 0xFF ? 2 * 0xFF : 0);
      menu->highlight_state[i] = -menu->highlight_state[i];
    }
    menu->highlight_color_animated &= ~mask;
    menu->highlight_color_animated |= (uint32_t)v << shift;
  }
  struct gfx_font *font = menu_get_font(menu, 1);
  uint8_t alpha = menu_get_alpha_i(menu, 1);
  if (!font)
    return;
  for (struct menu_item *item = menu->items.first;
       item; item = list_next(item))
  {
    if (item->draw_proc && item->draw_proc(item))
      continue;
    if (item->imenu)
      menu_draw(item->imenu);
    if (!item->text)
      continue;
    uint32_t color = (item == menu->selector ?
                      (item->animate_highlight ?
                       menu->highlight_color_animated :
                       menu->highlight_color_static) :
                      item->color);
    gfx_mode_color((color >> 16) & 0xFF,
                   (color >> 8)  & 0xFF,
                   (color >> 0)  & 0xFF,
                   alpha);
    gfx_printf(font,  menu_item_screen_x(item), menu_item_screen_y(item),
               "%s", item->text);
  }
}

static void menu_nav_compare(struct menu *menu,
                             struct menu_item *selector,
                             int nav_x, int nav_y,
                             struct menu_item **near_item,
                             struct menu_item **far_item,
                             int *npa, int *fpa, int *npe, int *fpe)
{
  if (menu->child)
    return menu_nav_compare(menu->child, selector, nav_x, nav_y,
                            near_item, far_item, npa, fpa, npe, fpe);
  int sel_x = selector ? menu_item_screen_x(selector) : 0;
  int sel_y = selector ? menu_item_screen_y(selector) : 0;
  for (struct menu_item *item = menu->items.first;
       item; item = list_next(item))
  {
    if (item->imenu)
      menu_nav_compare(item->imenu, selector, nav_x, nav_y,
                       near_item, far_item, npa, fpa, npe, fpe);
    if (item == menu->selector || !item->selectable)
      continue;
    int distance_x = menu_item_screen_x(item) - sel_x;
    int distance_y = menu_item_screen_y(item) - sel_y;
    int distance_pa = (nav_x ? distance_x * nav_x : distance_y * nav_y);
    int distance_pe = (nav_y ? distance_x : distance_y);
    if (distance_pe < 0)
      distance_pe = -distance_pe;
    if (distance_pa > 0 && (*near_item == NULL || distance_pa < *npa ||
                            (distance_pa == *npa && distance_pe < *npe)))
    {
      *npa = distance_pa;
      *npe = distance_pe;
      *near_item = item;
    }
    if (distance_pa < 0 && (*far_item == NULL || -distance_pa > *fpa ||
                            (-distance_pa == *fpa && distance_pe < *fpe)))
    {
      *fpa = -distance_pa;
      *fpe = distance_pe;
      *far_item = item;
    }
  }
}

void menu_navigate(struct menu *menu, enum menu_navigation nav)
{
  if (menu->child)
    return menu_navigate(menu->child, nav);
  if (menu->selector && menu->selector->navigate_proc &&
      menu->selector->navigate_proc(menu->selector, nav))
    return;
  int nav_x = (nav == MENU_NAVIGATE_LEFT ?
               -1 : (nav == MENU_NAVIGATE_RIGHT ? 1 : 0));
  int nav_y = (nav == MENU_NAVIGATE_UP ?
               -1 : (nav == MENU_NAVIGATE_DOWN ? 1 : 0));
  if (nav_x == 0 && nav_y == 0)
    return;
  int npa = 0;
  int fpa = 0;
  int npe = 0;
  int fpe = 0;
  struct menu_item *near_item = NULL;
  struct menu_item *far_item = NULL;
  menu_nav_compare(menu, menu->selector, nav_x, nav_y,
                   &near_item, &far_item, &npa, &fpa, &npe, &fpe);
  if (near_item) {
    if (menu->selector)
      menu->selector->owner->selector = NULL;
    menu->selector = near_item;
    near_item->owner->selector = near_item;
  }
  else if (far_item) {
    if (menu->selector)
      menu->selector->owner->selector = NULL;
    menu->selector = far_item;
    far_item->owner->selector = far_item;
  }
}

void menu_activate(struct menu *menu)
{
  if (menu->child)
    return menu_activate(menu->child);
  if (menu->selector && menu->selector->activate_proc)
    menu->selector->activate_proc(menu->selector);
}

void menu_enter(struct menu *menu, struct menu *submenu)
{
  if (menu->child)
    return menu_enter(menu->child, submenu);
  menu->child = submenu;
  submenu->parent = menu;
}

struct menu *menu_return(struct menu *menu)
{
  if (menu->child)
    return menu_return(menu->child);
  struct menu *parent = menu->parent;
  if (!parent || parent->child != menu)
    return NULL;
  menu->parent = NULL;
  parent->child = NULL;
  return parent;
}

void menu_navigate_top(struct menu *menu, enum menu_navigation nav)
{
  if (menu->parent)
    return menu_navigate_top(menu->parent, nav);
  return menu_navigate(menu, nav);
}

void menu_activate_top(struct menu *menu)
{
  if (menu->parent)
    return menu_activate_top(menu->parent);
  return menu_activate(menu);
}

void menu_enter_top(struct menu *menu, struct menu *submenu)
{
  if (menu->parent)
    return menu_enter_top(menu->parent, submenu);
  return menu_enter(menu, submenu);
}

struct menu *menu_return_top(struct menu *menu)
{
  if (menu->parent)
    return menu_return_top(menu->parent);
  return menu_return(menu);
}

struct menu_item *menu_item_add(struct menu *menu, int x, int y,
                                const char *text, uint32_t color)
{
  struct menu_item *item = list_push_back(&menu->items, NULL);
  if (item) {
    item->owner = menu;
    item->x = x;
    item->y = y;
    if (text) {
      item->text = malloc(strlen(text) + 1);
      strcpy(item->text, text);
    }
    else
      item->text = NULL;
    item->color = color;
    item->animate_highlight = 0;
    item->data = NULL;
    item->selectable = 1;
    item->imenu = NULL;
    item->think_proc = NULL;
    item->draw_proc = NULL;
    item->navigate_proc = NULL;
    item->activate_proc = NULL;
    item->destroy_proc = NULL;
  }
  return item;
}

static void menu_deselect(struct menu *menu, struct menu_item *item)
{
  if (menu->selector == item)
    menu->selector = NULL;
  if (menu->parent && menu->parent->child != menu)
    menu_deselect(menu->parent, item);
}

void menu_item_remove(struct menu_item *item)
{
  if (!item->destroy_proc || !item->destroy_proc(item)) {
    if (item->imenu) {
      menu_destroy(item->imenu);
      free(item->imenu);
    }
    if (item->text)
      free(item->text);
    if (item->data)
      free(item->data);
  }
  menu_deselect(item->owner, item);
  list_erase(&item->owner->items, item);
}

int menu_item_screen_x(struct menu_item *item)
{
  return menu_cell_screen_x(item->owner, item->x);
}

int menu_item_screen_y(struct menu_item *item)
{
  return menu_cell_screen_y(item->owner, item->y);
}

struct menu_item *menu_add_static(struct menu *menu, int x, int y,
                                  const char *text, uint32_t color)
{
  struct menu_item *item = menu_item_add(menu, x, y, text, color);
  item->selectable = 0;
  return item;
}

static int imenu_think_proc(struct menu_item *item)
{
  if (item->imenu) {
    item->imenu->cxoffset = item->x;
    item->imenu->cyoffset = item->y;
  }
  return 0;
}

static int imenu_navigate_proc(struct menu_item *item,
                               enum menu_navigation nav)
{
  if (item->imenu) {
    menu_navigate(item->imenu, nav);
    return 1;
  }
  return 0;
}

static int imenu_activate_proc(struct menu_item *item)
{
  if (item->imenu) {
    menu_activate(item->imenu);
    return 1;
  }
  return 0;
}

struct menu_item *menu_add_imenu(struct menu *menu, int x, int y,
                                 struct menu **p_imenu)
{
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0);
  item->selectable = 0;
  item->think_proc = imenu_think_proc;
  item->navigate_proc = imenu_navigate_proc;
  item->activate_proc = imenu_activate_proc;
  struct menu *imenu = malloc(sizeof(*imenu));
  menu_init(imenu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  imenu->parent = menu;
  item->imenu = imenu;
  if (p_imenu)
    *p_imenu = imenu;
  return item;
}