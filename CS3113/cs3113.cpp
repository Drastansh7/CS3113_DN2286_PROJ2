#include "cs3113.h"

Color ColorFromHex(const char *hex)
{
    if (hex[0] == '#') hex++;

    unsigned int r = 0, 
                 g = 0, 
                 b = 0, 
                 a = 255;

    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) == 3) {
        return (Color){ (unsigned char) r,
                        (unsigned char) g,
                        (unsigned char) b,
                        (unsigned char) a };
    }

    if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        return (Color){ (unsigned char) r,
                        (unsigned char) g,
                        (unsigned char) b,
                        (unsigned char) a };
    }

    return RAYWHITE;
}

float GetLength(const Vector2 vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y);
}

void Normalise(Vector2 *vector)
{
    float magnitude = GetLength(*vector);
    if (magnitude < 0.00001f) return; // avoid divide by zero

    vector->x /= magnitude;
    vector->y /= magnitude;
}
