#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GCODE_NONE = 0,
    GCODE_G0,   /* movimiento rápido */
    GCODE_G1    /* movimiento lineal a velocidad de trabajo */
} GcodeCommandType;

typedef struct {
    GcodeCommandType type;
    bool has_x;
    bool has_y;
    bool has_f;
    bool has_s;     /* potencia del láser (0..S_MAX) */
    bool has_z;
    bool has_g90;   /* la línea contiene G90 (modo absoluto) */
    bool has_g91;   /* la línea contiene G91 (modo relativo) */
    bool has_m3;    /* láser ON, potencia constante */
    bool has_m4;    /* láser ON, potencia dinámica (por ahora se trata igual que M3) */
    bool has_m5;    /* láser OFF */
    bool has_m84;   /* desactivar motores (liberar para mover a mano) */
    bool has_m17;   /* reactivar motores explícitamente */
    bool has_g92;   /* G92: fija el origen de trabajo (offset) */
    bool has_g92_1; /* G92.1: borra el offset de trabajo */
    bool has_unknown; /* la linea trae algo que el firmware no reconoce */
    float x;    /* mm, valor tal cual viene en la línea (absoluto o relativo según modo) */
    float y;    /* mm */
    float z;    /* mm */
    float f;    /* mm/min, como define el estándar G-code */
    float s;    /* potencia del láser, 0..S_MAX */
} GcodeCommand;

/* Parsea una línea de G-code (ya sin '\r\n', terminada en '\0').
 * Devuelve true si se reconoció un comando G0/G1 válido.
 * Ignora comentarios entre paréntesis y líneas que empiecen con ';'.
 */
bool gcode_parse_line(const char *line, GcodeCommand *out_cmd);

#endif /* GCODE_PARSER_H */
