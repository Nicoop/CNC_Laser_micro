#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include <stdint.h>
#include <stdbool.h>

/* tipo de movimiento que trae la linea. el resto de las cosas (x, y, z,
 * s, m-codigos) son banderas aparte porque en una sola linea de gcode
 * pueden venir varias cosas mezcladas, tipo "g1 x10 y5 s200" */
typedef enum {
    GCODE_NONE = 0,
    GCODE_G0,   /* movimiento rapido, sin laser */
    GCODE_G1    /* movimiento a la velocidad de trabajo (f) */
} GcodeCommandType;

typedef struct {
    GcodeCommandType type;
    bool has_x;
    bool has_y;
    bool has_f;
    bool has_s;     /* potencia del laser (0..s_max) */
    bool has_z;
    bool has_g90;   /* la linea trae g90 (modo absoluto) */
    bool has_g91;   /* la linea trae g91 (modo relativo) */
    bool has_m3;    /* laser on, potencia constante */
    bool has_m4;    /* laser on, potencia dinamica (por ahora lo trato igual que m3) */
    bool has_m5;    /* laser off */
    bool has_m84;   /* desactivar motores (para poder mover la maquina a mano) */
    bool has_m17;   /* reactivar motores */
    float x;    /* mm, tal cual viene en la linea (absoluto o relativo segun el modo activo) */
    float y;    /* mm */
    float z;    /* mm */
    float f;    /* mm/min, como pide el estandar de gcode */
    float s;    /* potencia del laser, 0..s_max */
} GcodeCommand;

/* parsea una linea de gcode (ya sin \r\n, terminada en \0).
 * devuelve true si encontro algun comando valido.
 * ignora comentarios entre parentesis y lineas que arrancan con ; */
bool gcode_parse_line(const char *line, GcodeCommand *out_cmd);

#endif /* GCODE_PARSER_H */
