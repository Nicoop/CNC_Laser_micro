#include "gcode_parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* nota sobre el modo absoluto/relativo (g90/g91): laser grbl por defecto
 * manda en absoluto. si en algun momento agrego g91 de verdad hay que
 * manejarlo donde se usa este parser (main.c), ac solo se detecta que
 * vino la g90/g91 en la linea, no se hace nada con eso */

/* ojo con esto: strtof de la libreria estandar interpreta "0x10" como
 * numero hexadecimal (0x10 = 16 decimal). esto rompe todo cuando llega
 * un gcode sin espacios tipo "g0x10", porque el "0" de la g0 queda
 * pegado a la x y strtof se come las dos cosas como si fueran un numero
 * hexa. me paso en serio y tarde bastante en encontrarlo. por eso hice
 * mi propio parser de numeros que solo entiende decimales normales y
 * nunca mastica la letra x como si fuera parte del numero */
static float parse_gcode_number(const char *str, char **end)
{
    const char *p = str;
    float sign = 1.0f;
    bool saw_digit = false;

    if (*p == '+' || *p == '-') {
        if (*p == '-') sign = -1.0f;
        p++;
    }

    float value = 0.0f;

    while (isdigit((unsigned char)*p)) {
        value = value * 10.0f + (float)(*p - '0');
        p++;
        saw_digit = true;
    }

    if (*p == '.') {
        p++;
        float frac = 0.1f;
        while (isdigit((unsigned char)*p)) {
            value += (float)(*p - '0') * frac;
            frac *= 0.1f;
            p++;
            saw_digit = true;
        }
    }

    if (!saw_digit) {
        /* no habia ningun digito real, asi que no es un numero valido */
        *end = (char *)str;
        return 0.0f;
    }

    *end = (char *)p;
    return value * sign;
}

bool gcode_parse_line(const char *line, GcodeCommand *out_cmd)
{
    if (line == NULL || out_cmd == NULL) return false;

    memset(out_cmd, 0, sizeof(GcodeCommand));
    out_cmd->type = GCODE_NONE;

    /* salteo espacios por si vienen al principio de la linea */
    while (*line == ' ' || *line == '\t') line++;

    /* lineas vacias o comentarios las ignoro directamente */
    if (*line == '\0' || *line == ';' || *line == '(') {
        return false;
    }

    bool found_command = false;

    /* recorro la linea letra por letra. cada letra (g, x, y, z, f, s, m)
     * viene seguida de un numero, y los voy guardando en la estructura
     * de salida segun corresponda */
    while (*line != '\0') {
        char letter = toupper((unsigned char)*line);

        if (letter == '(') {
            /* comentario entre parentesis, lo salto entero */
            while (*line != '\0' && *line != ')') line++;
            if (*line == ')') line++;
            continue;
        }

        if (letter == ';') {
            break; /* todo lo que sigue en la linea es comentario */
        }

        if (isalpha((unsigned char)letter)) {
            line++;
            char *end;
            float value = parse_gcode_number(line, &end);

            if (end == line) {
                /* la letra no tenia un numero valido atras, la salteo */
                continue;
            }

            switch (letter) {
                case 'G': {
                    int gcode_num = (int)value;
                    if (gcode_num == 0) {
                        out_cmd->type = GCODE_G0;
                        found_command = true;
                    } else if (gcode_num == 1) {
                        out_cmd->type = GCODE_G1;
                        found_command = true;
                    } else if (gcode_num == 90) {
                        out_cmd->has_g90 = true;
                        found_command = true;
                    } else if (gcode_num == 91) {
                        out_cmd->has_g91 = true;
                        found_command = true;
                    }
                    /* otros codigos g (g21, etc) los ignoro por ahora */
                    break;
                }
                case 'X':
                    out_cmd->has_x = true;
                    out_cmd->x = value;
                    break;
                case 'Y':
                    out_cmd->has_y = true;
                    out_cmd->y = value;
                    break;
                case 'Z':
                    out_cmd->has_z = true;
                    out_cmd->z = value;
                    break;
                case 'F':
                    out_cmd->has_f = true;
                    out_cmd->f = value;
                    break;
                case 'S':
                    out_cmd->has_s = true;
                    out_cmd->s = value;
                    break;
                case 'M': {
                    int mcode_num = (int)value;
                    if (mcode_num == 3) {
                        out_cmd->has_m3 = true;
                        found_command = true;
                    } else if (mcode_num == 4) {
                        out_cmd->has_m4 = true;
                        found_command = true;
                    } else if (mcode_num == 5) {
                        out_cmd->has_m5 = true;
                        found_command = true;
                    } else if (mcode_num == 84) {
                        out_cmd->has_m84 = true;
                        found_command = true;
                    } else if (mcode_num == 17) {
                        out_cmd->has_m17 = true;
                        found_command = true;
                    }
                    /* otros codigos m los ignoro por ahora */
                    break;
                }
                default:
                    /* letras que no uso todavia, las ignoro sin drama */
                    break;
            }

            line = end;
        } else {
            line++;
        }
    }

    /* esto es para el gcode modal: laser grbl a veces manda una linea
     * solo con "x17.333 s894" sin repetir el g1, porque en el estandar
     * de gcode eso significa "segui haciendo lo mismo que la linea
     * anterior". si no hago esto, esas lineas se ignoraban solas y el
     * raster se frenaba a la mitad sin razon aparente (me costo un rato
     * largo darme cuenta de esto) */
    if (!found_command && (out_cmd->has_x || out_cmd->has_y || out_cmd->has_z || out_cmd->has_f || out_cmd->has_s)) {
        found_command = true;
    }

    return found_command;
}
