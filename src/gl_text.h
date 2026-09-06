/*
 * Qt-based text rendering for GDIS OpenGL canvas
 * Replaces Cairo/Pango text rendering with QPainter/QImage
 */

#ifndef GL_TEXT_H
#define GL_TEXT_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize font from name string */
void gl_text_init_font(const gchar *fontname);

/* Render text to RGBA buffer (2D screen coordinates) */
void gl_text_render_2d(const gchar *str, unsigned char *buf, gint w, gint h, gint font_size, gint rotate, gdouble fg_r,
                       gdouble fg_g, gdouble fg_b);

/* Render text to RGBA buffer (3D world coordinates) */
void gl_text_render_3d(const gchar *str, unsigned char *buf, gint max_w, gint max_h, gint font_size, gdouble fg_r,
                       gdouble fg_g, gdouble fg_b, gint *out_width, gint *out_height);

/* Get approximate text width in pixels (default font) */
gint gl_text_width(const gchar *str);
/* Get approximate text width in pixels (custom font size) */
gint gl_text_width_size(const gchar *str, gint font_size);

/* Get font size in pixels */
gint gl_get_fontsize(void);

/* Get current font family name */
const gchar *gl_get_fontname(void);

/* alias for C core */
gint gl_get_fontsize_qt(void);

/* Render PNG snapshot using Qt */
void gl_render_snapshot(const char *filename, const unsigned char *pixels, int w, int h, const char *title,
                        const char *version_str);

#ifdef __cplusplus
}
#endif

#endif /* GL_TEXT_H */
