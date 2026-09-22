#include "settings.h"
#include "color.h"
#include "term/term.h"

#include <stddef.h>
#include <stdio.h>

enum field_kind { FIELD_NONE, FIELD_FPS, FIELD_PACE, FIELD_UTURN, FIELD_FISH, FIELD_SPECIES };

struct grid_cell {
  enum field_kind kind;
  const char *label;
  size_t species_offset;
};

#define SP(field) offsetof(struct aquatic_life, field)

#define GRID_ROWS 10

static const struct grid_cell grid[GRID_ROWS][2] = {
  {{FIELD_FPS, "fps", 0},        {FIELD_PACE, "pace", 0}},
  {{FIELD_UTURN, "uturn", 0},    {FIELD_NONE, NULL, 0}},
  {{FIELD_FISH, "fish", 0},      {FIELD_NONE, NULL, 0}},
  {{FIELD_SPECIES, "bigfish", SP(bigfish)},     {FIELD_SPECIES, "monster", SP(monster)}},
  {{FIELD_SPECIES, "crab", SP(crab)},           {FIELD_SPECIES, "shark", SP(shark)}},
  {{FIELD_SPECIES, "dolphins", SP(dolphins)},   {FIELD_SPECIES, "ship", SP(ship)}},
  {{FIELD_SPECIES, "ducks", SP(ducks)},         {FIELD_SPECIES, "submarine", SP(submarine)}},
  {{FIELD_SPECIES, "fishhook", SP(fishhook)},   {FIELD_SPECIES, "swan", SP(swan)}},
  {{FIELD_SPECIES, "jellyfish", SP(jellyfish)}, {FIELD_SPECIES, "swordfish", SP(swordfish)}},
  {{FIELD_SPECIES, "kaiju", SP(kaiju)},         {FIELD_SPECIES, "whale", SP(whale)}},
};

#undef SP

#define CONTENT_W 28
#define CONTENT_H 13
#define MARGIN 1
#define BOX_W (CONTENT_W + 2 * MARGIN)
#define BOX_H (CONTENT_H + 2 * MARGIN)
#define COL0_X 0
#define COL1_X 14

static const struct attr BOX_ATTR = {.col = COL_WHITE, .bold = false, .bg = COL_BLACK, .bg_bold = true};
static const struct attr HL_ATTR = {.col = COL_YELLOW, .bold = false, .bg = COL_BLUE, .bg_bold = false};

static int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static double clampd(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static bool *species_field(struct aquatic_life *a, size_t offset) {
  return (bool *)((char *)a + offset);
}

static int current_fish_value(const struct scene *sc) {
  return sc->aquatic.fish_count >= 0 ? sc->aquatic.fish_count : scene_fish_display_count(sc);
}

static int draw_row_for_logical(int lr) {
  if (lr == 0) return 2;
  if (lr == 1) return 3;
  if (lr == 2) return 5;
  return lr + 3;
}

void settings_ui_init(struct settings_ui *ui, int *fps, double *pace, struct scene *scene) {
  ui->open = false;
  ui->fps = fps;
  ui->pace = pace;
  ui->scene = scene;
  ui->sel_row = 0;
  ui->sel_col = 0;
}

bool settings_ui_is_open(const struct settings_ui *ui) { return ui->open; }

void settings_ui_toggle(struct settings_ui *ui) {
  ui->open = !ui->open;
  if (ui->open) {
    ui->sel_row = 0;
    ui->sel_col = 0;
  }
}

void settings_ui_close(struct settings_ui *ui) { ui->open = false; }

static void move_left_right(struct settings_ui *ui, int dcol) {
  int other = ui->sel_col + dcol;
  if (other < 0 || other > 1) return;
  if (grid[ui->sel_row][other].kind == FIELD_NONE) return;
  ui->sel_col = other;
}

static void move_up_down(struct settings_ui *ui, int drow) {
  int row = clampi(ui->sel_row + drow, 0, GRID_ROWS - 1);
  int col = ui->sel_col;
  if (grid[row][col].kind == FIELD_NONE) col = 0;
  ui->sel_row = row;
  ui->sel_col = col;
}

static void adjust(struct settings_ui *ui, int dir, int term_w, int term_h) {
  const struct grid_cell *cell = &grid[ui->sel_row][ui->sel_col];
  switch (cell->kind) {
    case FIELD_FPS:
      *ui->fps = clampi(*ui->fps + dir, 1, 120);
      break;
    case FIELD_PACE: {
      int hundredths = (int)(*ui->pace * 100.0 + (*ui->pace >= 0 ? 0.5 : -0.5)) + dir;
      *ui->pace = clampd((double)hundredths / 100.0, 0.01, 10.0);
      break;
    }
    case FIELD_UTURN:
      scene_set_uturn_chance(ui->scene, clampi(ui->scene->uturn_chance + dir, 0, 999));
      break;
    case FIELD_FISH: {
      int next = clampi(current_fish_value(ui->scene) + dir, 0, 999);
      scene_set_fish_count(ui->scene, term_w, term_h, next);
      break;
    }
    case FIELD_SPECIES:
    case FIELD_NONE:
    default:
      break;
  }
}

static void toggle_species(struct settings_ui *ui, int term_w, int term_h) {
  const struct grid_cell *cell = &grid[ui->sel_row][ui->sel_col];
  if (cell->kind != FIELD_SPECIES) return;
  bool *flag = species_field(&ui->scene->aquatic, cell->species_offset);
  *flag = !*flag;
  scene_on_species_toggled(ui->scene, term_w, term_h);
}

void settings_ui_handle_key(struct settings_ui *ui, int key, int term_w, int term_h) {
  if (!ui->open) return;
  switch (key) {
    case TERM_KEY_UP:    move_up_down(ui, -1);    break;
    case TERM_KEY_DOWN:  move_up_down(ui, 1);     break;
    case TERM_KEY_LEFT:  move_left_right(ui, -1); break;
    case TERM_KEY_RIGHT: move_left_right(ui, 1);  break;
    case '+':            adjust(ui, 1, term_w, term_h);  break;
    case '-':            adjust(ui, -1, term_w, term_h); break;
    case ' ':            toggle_species(ui, term_w, term_h); break;
    default: break;
  }
}

static void fill_rect(struct canvas *c, int x0, int y0, int w, int h, struct attr a) {
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      canvas_put(c, x0 + x, y0 + y, " ", 1, a, 1);
}

static void draw_row_text(struct canvas *c, int x0, int y, const char *s, int hl_start, int hl_len) {
  for (int i = 0; s[i] != '\0'; i++) {
    struct attr a = (hl_len > 0 && i >= hl_start && i < hl_start + hl_len) ? HL_ATTR : BOX_ATTR;
    char g[2] = {s[i], '\0'};
    canvas_put(c, x0 + i, y, g, 1, a, 1);
  }
}

void settings_ui_draw(const struct settings_ui *ui, struct canvas *c) {
  if (!ui->open) return;

  fill_rect(c, 0, 0, BOX_W, BOX_H, BOX_ATTR);
  draw_row_text(c, MARGIN, MARGIN + 0, "Settings", -1, 0);

  char line[CONTENT_W + 1];
  bool row0_sel = ui->sel_row == 0;
  snprintf(line, sizeof line, "%-7s-%3d+   %-5s-%5.2f+", "fps", *ui->fps, "pace", *ui->pace);
  int row0_off, row0_len;
  if (row0_sel && ui->sel_col == 0) { row0_off = 8; row0_len = 3; }
  else if (row0_sel && ui->sel_col == 1) { row0_off = 21; row0_len = 5; }
  else { row0_off = -1; row0_len = 0; }
  draw_row_text(c, MARGIN, MARGIN + draw_row_for_logical(0), line, row0_off, row0_len);

  snprintf(line, sizeof line, "%-7s-%3d+", "uturn", ui->scene->uturn_chance);
  draw_row_text(c, MARGIN, MARGIN + draw_row_for_logical(1), line, ui->sel_row == 1 ? 8 : -1, ui->sel_row == 1 ? 3 : 0);

  snprintf(line, sizeof line, "%-7s-%3d+", "fish", current_fish_value(ui->scene));
  draw_row_text(c, MARGIN, MARGIN + draw_row_for_logical(2), line, ui->sel_row == 2 ? 8 : -1, ui->sel_row == 2 ? 3 : 0);

  for (int lr = 3; lr < GRID_ROWS; lr++) {
    int y = MARGIN + draw_row_for_logical(lr);
    for (int col = 0; col < 2; col++) {
      const struct grid_cell *cell = &grid[lr][col];
      if (cell->kind == FIELD_NONE) continue;
      bool on = *species_field(&ui->scene->aquatic, cell->species_offset);
      char buf[16];
      snprintf(buf, sizeof buf, "[%c] %s", on ? 'x' : ' ', cell->label);
      bool sel = ui->sel_row == lr && ui->sel_col == col;
      draw_row_text(c, MARGIN + (col == 0 ? COL0_X : COL1_X), y, buf, sel ? 0 : -1, sel ? 3 : 0);
    }
  }
}
