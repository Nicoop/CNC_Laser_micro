#include "gcode_parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* strtof() interpreta un prefijo "0x"/"0X" como notación hexadecimal
 * (ej. "0X10" -> 16.0), lo cual es un desastre para G-code sin espacios
 * como "G0X10" (el '0' de G0 pegado a la X). Esta versión propia SOLO
 * entiende números decimales normales: signo opcional, dígitos, punto
 * decimal opcional, dígitos. Nunca interpreta 'x'/'X' como parte del
 * número, así que corta ahí y deja la letra siguiente para el llamador.
 */
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
        /* no había ningún dígito real: número inválido */
        *end = (char *)str;
        return 0.0f;
    }

    *end = (char *)p;
    return value * sign;
}

/* Estado interno: LaserGRBL por defecto envía coordenadas absolutas (G90).
 * Si más adelante agregás G91 (relativo), este módulo es el lugar para manejarlo.
 */

bool gcode_parse_line(const char *line, GcodeCommand *out_cmd)
{
    if (line == NULL || out_cmd == NULL) return false;

    memset(out_cmd, 0, sizeof(GcodeCommand));
    out_cmd->type = GCODE_NONE;

    /* Saltar espacios iniciales */
    while (*line == ' ' || *line == '\t') line++;

    /* Ignorar líneas vacías o comentarios */
    if (*line == '\0' || *line == ';' || *line == '(') {
        return false;
    }

    bool found_command = false;

    while (*line != '\0') {
        char letter = toupper((unsigned char)*line);

        if (letter == '(') {
            /* saltar comentario entre paréntesis */
            while (*line != '\0' && *line != ')') line++;
            if (*line == ')') line++;
            continue;
        }

        if (letter == ';') {
            break; /* resto de la línea es comentario */
        }

        if (isalpha((unsigned char)letter)) {
            line++;
            char *end;
            float value = parse_gcode_number(line, &end);

            if (end == line) {
                /* letra sin número válido detrás, la salteamos */
                continue;
            }

            switch (letter) {
                case 'G': {
                    int gcode_num = (int)value;
                    if (value > 91.99f && value < 92.05f) {
                        out_cmd->has_g92 = true;
                        found_command = true;
                    } else if (value > 92.05f && value < 92.15f) {
                        out_cmd->has_g92_1 = true;
                        found_command = true;
                    } else if (gcode_num == 0) {
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
                    } else {
                        out_cmd->has_unknown = true;
                    }
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
                    } else {
                        out_cmd->has_unknown = true;
                    }
                    break;
                }
                default:
                    out_cmd->has_unknown = true; /* letra que no maneja el firmware */
                    break;
            }

            line = end;
        } else {
            line++;
        }
    }

    /* G-code modal: una línea con X/Y/F pero sin G explícito sigue siendo
     * un comando válido (continúa el último tipo de movimiento). */
    if (!found_command && (out_cmd->has_x || out_cmd->has_y || out_cmd->has_z || out_cmd->has_f || out_cmd->has_s)) {
        found_command = true;
    }

    return found_command || out_cmd->has_unknown;
}
