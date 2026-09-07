/*
 * Qt-based text rendering for GDIS OpenGL canvas
 * Replaces Cairo/Pango text rendering with QPainter/QImage
 */

#include "gl_text.h"
#include <glib.h>
#include <QFontDatabase>
#include <QPainter>
#include <QImage>
#include <QString>
#include <cstring>

static QFont g_font;
static gint g_fontsize = 12;
static bool g_font_initialized = false;

static void ensure_font_initialized()
{
  if (!g_font_initialized)
  {
    g_font = QFont("Sans Serif");
    g_font.setPixelSize(12);
    g_fontsize = 12;
    g_font_initialized = true;
  }
}

/* Helper: convert gchar* to QString */
static QString toQString(const char *str) { return QString::fromUtf8(str, strlen(str)); }

/* Strip Pango/HTML markup tags like <big>, </small>, <b>, <i>, etc. */
static QString strip_pango_markup(const char *str)
{
  QString result;
  const char *p = str;
  bool in_tag = false;
  while (*p)
  {
    if (*p == '<')
    {
      in_tag = true;
      p++;
      continue;
    }
    if (*p == '>')
    {
      in_tag = false;
      p++;
      continue;
    }
    if (!in_tag)
    {
      result.append(QChar(*p));
      p++;
    } else
    {
      /* Skip tag content */
      while (*p && *p != '>')
        p++;
    }
  }
  return result;
}

/* Convert HTML numeric entities like &#952; to Unicode characters */
static QString decode_html_entities(const char *str)
{
  QString result;
  const char *p = str;
  while (*p)
  {
    if (*p == '&' && *(p + 1) == '#')
    {
      /* Try numeric entity: &#NNN; or &#xHH; */
      const char *end = p + 2;
      int base = 10;
      if (*end == 'x' || *end == 'X')
      {
        base = 16;
        end++;
      }
      char *endptr;
      long codepoint = strtol(end, &endptr, base);
      if (endptr > end && *endptr == ';')
      {
        result.append(QChar((ushort) codepoint));
        p = endptr + 1;
        continue;
      }
    }
    result.append(QChar(*p));
    p++;
  }
  return result;
}

/* Full cleanup: strip Pango markup then decode HTML entities */
static QString clean_text(const char *str)
{
  QString stripped = strip_pango_markup(str);
  return decode_html_entities(stripped.toUtf8().constData());
}

extern "C" {

void gl_text_init_font(const gchar *fontname)
{
  ensure_font_initialized();
  g_font = QFont(QString::fromUtf8(fontname));
  g_font.setPixelSize(12);
  g_fontsize = 12;
}

gint gl_get_fontsize(void)
{
  ensure_font_initialized();
  return g_fontsize;
}

const gchar *gl_get_fontname(void)
{
  ensure_font_initialized();
  static gchar cached_name[256];
  strncpy(cached_name, g_font.family().toUtf8().constData(), 255);
  cached_name[255] = '\0';
  return cached_name;
}

/* alias for C core */
gint gl_get_fontsize_qt(void) { return g_fontsize; }

gint gl_text_width(const gchar *str)
{
  QFontMetrics fm(g_font);
  return fm.horizontalAdvance(QString::fromUtf8(str));
}

gint gl_text_width_size(const gchar *str, gint font_size)
{
  ensure_font_initialized();
  QFont f = g_font;
  f.setPixelSize(font_size);
  QFontMetrics fm(f);
  return fm.horizontalAdvance(clean_text(str));
}

void gl_text_render_2d(const gchar *str, unsigned char *buf, gint w, gint h, gint font_size, gint rotate, gdouble fg_r,
                       gdouble fg_g, gdouble fg_b)
{
  ensure_font_initialized();
  QImage img(buf, w, h, w * 4, QImage::Format_ARGB32);
  img.fill(0x00000000);

  QPainter painter(&img);
  painter.setRenderHint(QPainter::Antialiasing);

  QFont f = g_font;
  f.setPixelSize(font_size);
  painter.setFont(f);

  painter.setPen(QColor(qBound(0, (int) (fg_r * 255), 255), qBound(0, (int) (fg_g * 255), 255),
                        qBound(0, (int) (fg_b * 255), 255)));

  QFontMetrics fm(f);
  QString qstr = clean_text(str);
  int text_w = fm.horizontalAdvance(qstr);
  int text_h = fm.height();

  if (rotate != 0)
  {
    /* For rotated text: translate to center, rotate, draw text,
     * restore. The square buffer guarantees the rotated result
     * stays in bounds. */
    painter.save();
    painter.translate(w / 2.0, h / 2.0);
    painter.rotate(-rotate);
    painter.drawText(0, 0, qstr);
    painter.restore();
  } else
  {
    /* Draw text right-side-up in buffer. Baseline at fm.height() from top.
     * Clamp to ensure text stays within buffer bounds. */
    int baseline = qMin(fm.height(), h - 1);
    painter.drawText(0, baseline, qstr);
  }

  painter.end();
}

void gl_text_render_3d(const gchar *str, unsigned char *buf, gint max_w, gint max_h, gint font_size, gdouble fg_r,
                       gdouble fg_g, gdouble fg_b, gint *out_width, gint *out_height)
{
  QFont f = g_font;
  f.setPixelSize(font_size);
  QFontMetrics fm(f);

  QString qstr = clean_text(str);
  int text_w = fm.horizontalAdvance(qstr);
  int text_h = fm.height();

  /* Clamp to max dimensions */
  if (text_w > max_w)
    text_w = max_w;
  if (text_h > max_h)
    text_h = max_h;
  if (text_w < 1)
    text_w = 1;
  if (text_h < 1)
    text_h = 1;

  QImage img(buf, text_w, text_h, text_w * 4, QImage::Format_ARGB32);
  img.fill(0x00000000);

  QPainter painter(&img);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setFont(f);
  painter.setPen(QColor(qBound(0, (int) (fg_r * 255), 255), qBound(0, (int) (fg_g * 255), 255),
                        qBound(0, (int) (fg_b * 255), 255)));

  QRect rect(0, 0, text_w, text_h);
  painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, qstr);
  painter.end();

  *out_width = text_w;
  *out_height = text_h;
}

void gl_render_snapshot(const char *filename, const unsigned char *pixels, int w, int h, const char *title,
                        const char *version_str)
{
  QImage img(pixels, w, h, w * 4, QImage::Format_ARGB32);
  img = img.convertToFormat(QImage::Format_RGB32);

  /* Save as PNG (always available) */
  QString path = toQString(filename);
  img.save(path, "PNG");
}

} /* extern "C" */
