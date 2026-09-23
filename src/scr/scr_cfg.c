#include "scr.h"
#include "scr_res.h"
#include "../opts.h"
#include "../version.h"
#include "../xalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_PATH L"Software\\underthec\\Screensaver"

enum reg_opt { OPT_CLASSIC, OPT_AQUATIC, OPT_MESSAGE, OPT_COLOR, OPT_POSITION, OPT_PACE, OPT_UTURN, OPT_FPS, OPT_COUNT };

/* long CLI names */
static const char *const opt_names[OPT_COUNT] = {
  "classic", "aquatic-life", "message", "message-color", "message-position", "pace", "uturn-chance", "fps",
};

/* dialog names in errors */
static const char *const opt_labels[OPT_COUNT] = {
  "Classic", "Fish", "Message", "Color", "Position", "Pace", "U-turn chance", "FPS",
};

static const char *const classic_items[] = {"off", "1.0", "1.1"};
static const char *const color_items[] = {
  "(default)", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
  "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
};
/* enum message_position order */
static const char *const position_items[] = {"middle", "center", "marquee", "swim", "event"};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

_Static_assert(SCENE_AQUATIC_FLAG_COUNT == SCR_FLAG_SLOTS, "dialog checkboxes out of sync with creatures");

/* shown over visible dialog */
#define WM_LOAD_FAILED (WM_APP + 1)

static HFONT message_font;
static char load_err[256];

static char *utf8_from_wide(const WCHAR *w) {
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  char *s = xmalloc(n > 0 ? (size_t)n : 1);
  if (n <= 0 || WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL) <= 0) s[0] = '\0';
  return s;
}

static WCHAR *wide_from_utf8(const char *s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  WCHAR *w = xmalloc((n > 0 ? (size_t)n : 1) * sizeof(*w));
  if (n <= 0 || MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) <= 0) w[0] = L'\0';
  return w;
}

static char *reg_read(HKEY key, const char *name) {
  WCHAR *wname = wide_from_utf8(name);
  DWORD type = 0;
  DWORD size = 0;
  char *out = NULL;
  if (RegQueryValueExW(key, wname, NULL, &type, NULL, &size) == ERROR_SUCCESS && type == REG_SZ) {
    WCHAR *buf = xcalloc(size / sizeof(WCHAR) + 1, sizeof(WCHAR));
    if (RegQueryValueExW(key, wname, NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) out = utf8_from_wide(buf);
    free(buf);
  }
  free(wname);
  return out;
}

bool scr_config_load(struct config *cfg, char *err, size_t err_len) {
  HKEY key;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &key) != ERROR_SUCCESS) return true;
  bool ok = true;
  for (size_t i = 0; i < OPT_COUNT && ok; i++) {
    char *val = reg_read(key, opt_names[i]);
    if (val == NULL) continue;
    ok = config_set(cfg, opt_names[i], val, opt_names[i], err, err_len);
    free(val);
  }
  RegCloseKey(key);
  return ok;
}

/* NULL val: delete */
static bool config_save(char *const vals[OPT_COUNT]) {
  HKEY key;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS) return false;
  bool ok = true;
  for (size_t i = 0; i < OPT_COUNT; i++) {
    WCHAR *wname = wide_from_utf8(opt_names[i]);
    if (vals[i] == NULL) {
      RegDeleteValueW(key, wname);
    } else {
      WCHAR *wval = wide_from_utf8(vals[i]);
      DWORD size = (DWORD)((wcslen(wval) + 1) * sizeof(WCHAR));
      if (RegSetValueExW(key, wname, 0, REG_SZ, (const BYTE *)wval, size) != ERROR_SUCCESS) ok = false;
      free(wval);
    }
    free(wname);
  }
  RegCloseKey(key);
  return ok;
}

static char *item_text(HWND dlg, int id) {
  HWND ctl = GetDlgItem(dlg, id);
  int len = GetWindowTextLengthW(ctl);
  WCHAR *w = xcalloc((size_t)len + 1, sizeof(WCHAR));
  GetWindowTextW(ctl, w, len + 1);
  char *s = utf8_from_wide(w);
  free(w);
  return s;
}

static void item_set_text(HWND dlg, int id, const char *s) {
  WCHAR *w = wide_from_utf8(s);
  SetDlgItemTextW(dlg, id, w);
  free(w);
}

static void combo_fill(HWND dlg, int id, const char *const *items, size_t count) {
  for (size_t i = 0; i < count; i++) SendDlgItemMessageA(dlg, id, CB_ADDSTRING, 0, (LPARAM)items[i]);
}

static int combo_sel(HWND dlg, int id) {
  LRESULT sel = SendDlgItemMessageW(dlg, id, CB_GETCURSEL, 0, 0);
  return sel == CB_ERR ? 0 : (int)sel;
}

static bool checked(HWND dlg, int id) { return IsDlgButtonChecked(dlg, id) == BST_CHECKED; }

/* -c and -a exclusive */
static void update_enabled(HWND dlg) {
  bool free_life = combo_sel(dlg, IDC_CLASSIC) == 0;
  EnableWindow(GetDlgItem(dlg, IDC_FISH_AUTO), free_life);
  EnableWindow(GetDlgItem(dlg, IDC_FISH), free_life && !checked(dlg, IDC_FISH_AUTO));
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) EnableWindow(GetDlgItem(dlg, IDC_FLAG0 + (int)i), free_life);
}

/* LF to CRLF for edit control */
static char *crlf_from_lf(const char *s) {
  char *out = xmalloc(strlen(s) * 2 + 1);
  char *o = out;
  for (; *s != '\0'; s++) {
    if (*s == '\n') *o++ = '\r';
    *o++ = *s;
  }
  *o = '\0';
  return out;
}

static void strip_cr(char *s) {
  char *o = s;
  for (; *s != '\0'; s++)
    if (*s != '\r') *o++ = *s;
  *o = '\0';
}

static void controls_from_config(HWND dlg, struct config *cfg) {
  char buf[32];
  SendDlgItemMessageW(dlg, IDC_CLASSIC, CB_SETCURSEL, (WPARAM)cfg->classic_ver, 0);
  bool fish_auto = cfg->aquatic.fish_count < 0;
  CheckDlgButton(dlg, IDC_FISH_AUTO, fish_auto ? BST_CHECKED : BST_UNCHECKED);
  snprintf(buf, sizeof buf, "%d", fish_auto ? 0 : cfg->aquatic.fish_count);
  item_set_text(dlg, IDC_FISH, fish_auto ? "" : buf);
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    bool on = *scene_aquatic_flag(&cfg->aquatic, i);
    CheckDlgButton(dlg, IDC_FLAG0 + (int)i, on ? BST_CHECKED : BST_UNCHECKED);
  }
  char *msg = crlf_from_lf(cfg->message != NULL ? cfg->message : "");
  item_set_text(dlg, IDC_MESSAGE, msg);
  free(msg);
  int color = 0;
  for (size_t i = 1; cfg->message_color != NULL && i < COUNT(color_items); i++)
    if (strcmp(cfg->message_color, color_items[i]) == 0) color = (int)i;
  SendDlgItemMessageW(dlg, IDC_MSG_COLOR, CB_SETCURSEL, (WPARAM)color, 0);
  SendDlgItemMessageW(dlg, IDC_MSG_POS, CB_SETCURSEL, (WPARAM)cfg->message_position, 0);
  snprintf(buf, sizeof buf, "%.2f", cfg->pace);
  char *end = buf + strlen(buf);
  while (end[-1] == '0') *--end = '\0';
  if (end[-1] == '.') end[-1] = '\0';
  item_set_text(dlg, IDC_PACE, buf);
  snprintf(buf, sizeof buf, "%d", cfg->fps);
  item_set_text(dlg, IDC_FPS, buf);
  snprintf(buf, sizeof buf, "%d", cfg->uturn_chance);
  item_set_text(dlg, IDC_UTURN, buf);
  update_enabled(dlg);
}

static char *aquatic_definition(HWND dlg) {
  char *fish = checked(dlg, IDC_FISH_AUTO) ? opts_strdup("auto") : item_text(dlg, IDC_FISH);
  size_t cap = strlen(fish) + 8;
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) cap += strlen(scene_aquatic_flag_name(i)) + 1;
  char *def = xmalloc(cap);
  size_t pos = 0;
  opts_append_bounded(def, cap, &pos, "fish=");
  opts_append_bounded(def, cap, &pos, fish);
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    if (!checked(dlg, IDC_FLAG0 + (int)i)) continue;
    opts_append_bounded(def, cap, &pos, ",");
    opts_append_bounded(def, cap, &pos, scene_aquatic_flag_name(i));
  }
  def[pos] = '\0';
  free(fish);
  return def;
}

static bool apply(HWND dlg) {
  char *vals[OPT_COUNT] = {0};
  int classic = combo_sel(dlg, IDC_CLASSIC);
  if (classic > 0) vals[OPT_CLASSIC] = opts_strdup(classic_items[classic]);
  else vals[OPT_AQUATIC] = aquatic_definition(dlg);
  vals[OPT_MESSAGE] = item_text(dlg, IDC_MESSAGE);
  strip_cr(vals[OPT_MESSAGE]);
  if (vals[OPT_MESSAGE][0] == '\0') {
    free(vals[OPT_MESSAGE]);
    vals[OPT_MESSAGE] = NULL;
  }
  int color = combo_sel(dlg, IDC_MSG_COLOR);
  if (color > 0) vals[OPT_COLOR] = opts_strdup(color_items[color]);
  vals[OPT_POSITION] = opts_strdup(position_items[combo_sel(dlg, IDC_MSG_POS)]);
  vals[OPT_PACE] = item_text(dlg, IDC_PACE);
  vals[OPT_UTURN] = item_text(dlg, IDC_UTURN);
  vals[OPT_FPS] = item_text(dlg, IDC_FPS);

  char err[256];
  struct config cfg;
  config_init(&cfg);
  bool ok = true;
  for (size_t i = 0; i < OPT_COUNT && ok; i++)
    if (vals[i] != NULL) ok = config_set(&cfg, opt_names[i], vals[i], opt_labels[i], err, sizeof err);
  if (ok) ok = config_check(&cfg, err, sizeof err);
  config_free(&cfg);
  if (!ok) {
    WCHAR *w = wide_from_utf8(err);
    MessageBoxW(dlg, w, L"underthec", MB_OK | MB_ICONWARNING);
    free(w);
  } else if (!config_save(vals)) {
    MessageBoxW(dlg, L"Could not save settings to the registry.", L"underthec", MB_OK | MB_ICONERROR);
    ok = false;
  }
  for (size_t i = 0; i < OPT_COUNT; i++) free(vals[i]);
  return ok;
}

typedef UINT(WINAPI *dpi_for_window_fn)(HWND);

/* GetDpiForWindow: Win 10 1607+(?) */
static UINT window_dpi(HWND wnd) {
  FARPROC p = GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
  if (p != NULL) return ((dpi_for_window_fn)(void (*)(void))p)(wnd);
  HDC dc = GetDC(wnd);
  UINT dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSY);
  ReleaseDC(wnd, dc);
  return dpi;
}

/* WM_SETFONT fonts not rescaled by dialog manager */
static void message_font_set(HWND dlg, UINT dpi) {
  HFONT old = message_font;
  message_font = CreateFontW(-MulDiv(9, (int)dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
  SendDlgItemMessageW(dlg, IDC_MESSAGE, WM_SETFONT, (WPARAM)message_font, TRUE);
  if (old != NULL) DeleteObject(old);
}

static void init_dialog(HWND dlg) {
  combo_fill(dlg, IDC_CLASSIC, classic_items, COUNT(classic_items));
  combo_fill(dlg, IDC_MSG_COLOR, color_items, COUNT(color_items));
  combo_fill(dlg, IDC_MSG_POS, position_items, COUNT(position_items));
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) item_set_text(dlg, IDC_FLAG0 + (int)i, scene_aquatic_flag_name(i));
  message_font_set(dlg, window_dpi(dlg));
  item_set_text(dlg, IDC_VERSION, TOOL_NAME " v" TOOL_VERSION);

  struct config cfg;
  config_init(&cfg);
  if (!scr_config_load(&cfg, load_err, sizeof load_err)) {
    config_free(&cfg);
    config_init(&cfg);
    PostMessageW(dlg, WM_LOAD_FAILED, 0, 0);
  }
  controls_from_config(dlg, &cfg);
  config_free(&cfg);
}

static void show_load_error(HWND dlg) {
  ShowWindow(dlg, SW_SHOW);
  UpdateWindow(dlg);
  char msg[320];
  snprintf(msg, sizeof msg, "Stored settings are invalid, showing defaults.\n\n%s", load_err);
  WCHAR *w = wide_from_utf8(msg);
  MessageBoxW(dlg, w, L"underthec", MB_OK | MB_ICONWARNING);
  free(w);
}

static INT_PTR CALLBACK dlg_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_INITDIALOG:
      init_dialog(dlg);
      return TRUE;
    case WM_DPICHANGED:
      message_font_set(dlg, HIWORD(wp));
      return FALSE;
    case WM_LOAD_FAILED:
      show_load_error(dlg);
      return TRUE;
    case WM_CTLCOLORSTATIC:
      if ((HWND)lp == GetDlgItem(dlg, IDC_VERSION)) {
        SetTextColor((HDC)wp, GetSysColor(COLOR_GRAYTEXT));
        SetBkMode((HDC)wp, TRANSPARENT);
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
      }
      return FALSE;
    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case IDC_CLASSIC:
          if (HIWORD(wp) == CBN_SELCHANGE) update_enabled(dlg);
          return TRUE;
        case IDC_FISH_AUTO:
          update_enabled(dlg);
          return TRUE;
        case IDC_DEFAULTS: {
          struct config cfg;
          config_init(&cfg);
          controls_from_config(dlg, &cfg);
          return TRUE;
        }
        case IDOK:
          if (apply(dlg)) EndDialog(dlg, IDOK);
          return TRUE;
        case IDCANCEL:
          EndDialog(dlg, IDCANCEL);
          return TRUE;
        default:
          return FALSE;
      }
    case WM_DESTROY:
      if (message_font != NULL) DeleteObject(message_font);
      message_font = NULL;
      return FALSE;
    default:
      return FALSE;
  }
}

void scr_config_dialog(HINSTANCE inst, HWND parent) {
  DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_CONFIG), parent, dlg_proc, 0);
}
