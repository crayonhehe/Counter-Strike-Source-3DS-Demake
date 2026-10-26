#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WHITE C2D_Color32(240, 240, 235, 255)
#define BLACK C2D_Color32(10, 12, 14, 255)
#define DARK  C2D_Color32(20, 24, 27, 255)
#define PANEL C2D_Color32(38, 42, 47, 240)
#define SAND  C2D_Color32(185, 152, 101, 255)
#define WALL  C2D_Color32(118, 94, 66, 255)
#define EDGE  C2D_Color32(196, 164, 103, 255)
#define RED   C2D_Color32(201, 70, 55, 255)
#define GREEN C2D_Color32(84, 190, 110, 255)
#define BLUE  C2D_Color32(66, 133, 220, 255)
#define YELLOW C2D_Color32(232, 210, 100, 255)
#define SKY   C2D_Color32(109, 154, 188, 255)
#define GRASS C2D_Color32(69, 121, 83, 255)

static C2D_TextBuf textBuf;
static C2D_Text hudText;

static float playerX = 0.0f, playerY = 0.0f, yaw = 0.0f, pitch = 0.0f;
static int health = 100, armour = 0, ammo = 20, reserveAmmo = 120, money = 800;
static int aiming = 0, scoped = 0, planted = 0, reloadFrames = 0, fireCooldown = 0;
static int weapon = 2, showBuy = 0, roundFrames = 5400, bombFrames = 0, roundState = 0, kills = 0;
static int botThink = 0;
static const char *weaponNames[] = {"AK-47", "AWP", "GLOCK"};

typedef struct {
    const char *name;
    float x, y;
    int hp, alive, team;
} Bot;

/* Team 0 = Terrorist ally, team 1 = Counter-Terrorist opponent. */
static Bot bots[] = {
    {"poo ringo", -1.0f, -1.0f, 100, 1, 0},
    {"poo shark", -2.0f, 0.6f, 100, 1, 0},
    {"poo zach", 0.2f, -2.0f, 100, 1, 0},
    {"CT Alpha", 0.45f, 0.1f, 100, 1, 1},
    {"CT Bravo", 2.2f, 1.1f, 100, 1, 1},
    {"CT Charlie", -2.2f, 1.7f, 100, 1, 1},
    {"CT Delta", 1.1f, -2.5f, 100, 1, 1}
};

#define BOT_COUNT ((int)(sizeof(bots) / sizeof(bots[0])))

static touchPosition prevTouch;
static bool wasTouching = false;

static void rect(C3D_RenderTarget *target, float x, float y, float w, float h, u32 color) {
    (void)target;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, color);
}

static void line(float x1, float y1, float x2, float y2, float width, u32 color) {
    C2D_DrawLine(x1, y1, color, x2, y2, color, width, 0.0f);
}

static void circleOutline(float cx, float cy, float radius, float width, u32 color) {
    const int segments = 48;
    float px = cx + radius;
    float py = cy;
    for (int i = 1; i <= segments; ++i) {
        float a = (float)i * 6.28318530718f / (float)segments;
        float nx = cx + cosf(a) * radius;
        float ny = cy + sinf(a) * radius;
        C2D_DrawLine(px, py, color, nx, ny, color, width, 0.0f);
        px = nx;
        py = ny;
    }
}

static void drawText(float x, float y, float scale, u32 color, const char *s) {
    C2D_TextParse(&hudText, textBuf, s);
    C2D_TextOptimize(&hudText);
    C2D_DrawText(&hudText, C2D_WithColor, x, y, 0.0f, scale, scale, color);
}

static int aliveEnemies(void) {
    int n = 0;
    for (int i = 0; i < BOT_COUNT; ++i) {
        if (bots[i].team == 1 && bots[i].alive) {
            ++n;
        }
    }
    return n;
}

static void resetRound(void) {
    health = 100;
    armour = 0;
    weapon = 2;
    ammo = 20;
    reserveAmmo = 120;
    money = 800;
    playerX = 0.0f;
    playerY = 0.0f;
    yaw = 0.0f;
    pitch = 0.0f;
    planted = 0;
    bombFrames = 0;
    roundFrames = 5400;
    roundState = 0;
    showBuy = 0;
    kills = 0;

    for (int i = 0; i < BOT_COUNT; ++i) {
        bots[i].alive = 1;
        bots[i].hp = 100;
        if (bots[i].team == 1) {
            bots[i].x = (i - 3) * 0.9f;
            bots[i].y = (i % 2) ? 1.3f : -1.2f;
        }
    }
}

static void buyAt(int row) {
    if (roundFrames < 5100) {
        return;
    }

    if (row == 0) {
        weapon = 2;
        ammo = 20;
        reserveAmmo = 90;
    } else if (row == 1 && money >= 2700) {
        money -= 2700;
        weapon = 0;
        ammo = 30;
        reserveAmmo = 90;
    } else if (row == 2 && money >= 4750) {
        money -= 4750;
        weapon = 1;
        ammo = 10;
        reserveAmmo = 30;
    } else if (row == 3 && money >= 650 && armour < 100) {
        money -= 650;
        armour = 100;
    }
    showBuy = 0;
}

static void drawBuyMenu(void) {
    rect(NULL, 0, 0, 400, 240, DARK);
    drawText(12, 8, 0.72f, YELLOW, "BUY MENU   |   X TO CLOSE");

    char moneyText[48];
    snprintf(moneyText, sizeof(moneyText), "MONEY: $%d", money);
    drawText(12, 30, 0.62f, WHITE, moneyText);

    int rows[4][2] = {{10, 55}, {10, 94}, {10, 133}, {10, 172}};
    const char *labels[4] = {
        "1  GLOCK  -  FREE / $200 AMMO",
        "2  AK-47  -  $2700",
        "3  AWP  -  $4750",
        "4  ARMOUR  -  $650"
    };

    for (int i = 0; i < 4; ++i) {
        rect(NULL, rows[i][0], rows[i][1], 300, 34, PANEL);
        drawText(rows[i][0] + 8, rows[i][1] + 9, 0.58f, WHITE, labels[i]);
    }

    drawText(12, 218, 0.48f, WHITE, "Tap a row during the buy phase");
}

static void drawScope(void) {
    rect(NULL, 0, 0, 400, 240, BLACK);
    rect(NULL, 0, 0, 400, 240, C2D_Color32(14, 14, 14, 255));

    C2D_DrawCircleSolid(200, 120, 0.0f, 96.0f, C2D_Color32(12, 12, 12, 255));
    C2D_DrawCircleSolid(200, 120, 0.0f, 88.0f, C2D_Color32(75, 74, 72, 255));
    circleOutline(200, 120, 96.0f, 2.0f, WHITE);
    line(110, 120, 290, 120, 1.0f, BLACK);
    line(200, 30, 200, 210, 1.0f, BLACK);
    C2D_DrawCircleSolid(200, 120, 0.0f, 2.5f, RED);
    drawText(12, 12, 0.52f, WHITE, "AWP / ZOOM 1X / 2X");
}

static void drawWorld(void) {
    rect(NULL, 0, 0, 400, 240, SKY);
    rect(NULL, 0, 132, 400, 108, SAND);

    /* Ground plane */
    line(0, 240, 140, 112, 2.0f, C2D_Color32(120, 90, 60, 255));
    line(400, 240, 260, 112, 2.0f, C2D_Color32(120, 90, 60, 255));
    line(140, 112, 260, 112, 2.0f, EDGE);

    /* Long forward corridor */
    rect(NULL, 0, 65, 110, 82, WALL);
    rect(NULL, 290, 58, 110, 89, WALL);
    rect(NULL, 0, 65, 110, 4, EDGE);
    rect(NULL, 290, 58, 110, 4, EDGE);

    /* Mid-area room */
    rect(NULL, 150, 72, 100, 70, C2D_Color32(87, 67, 47, 255));
    rect(NULL, 162, 82, 76, 48, C2D_Color32(48, 42, 35, 255));

    /* Props */
    rect(NULL, 46, 150, 52, 34, C2D_Color32(116, 78, 44, 255));
    rect(NULL, 50, 154, 44, 26, C2D_Color32(146, 100, 54, 255));
    line(50, 154, 94, 178, 2.0f, WALL);

    /* Bombsite markers */
    rect(NULL, 298, 122, 24, 20, RED);
    rect(NULL, 84, 120, 24, 20, RED);

    for (int i = 0; i < BOT_COUNT; ++i) {
        Bot *b = &bots[i];
        if (!b->alive) {
            continue;
        }

        float tx = 200.0f + (b->x - playerX - yaw * 0.20f) * 48.0f;
        float ty = 118.0f - (b->y - playerY - pitch * 0.02f) * 26.0f;

        if (tx > 12 && tx < 388) {
            u32 teamColor = b->team == 1 ? BLUE : RED;
            float scale = 1.0f / (1.0f + fabsf(b->x - playerX) * 0.1f + fabsf(b->y - playerY) * 0.1f);
            float bw = 12.0f * scale;
            float bh = 22.0f * scale;
            rect(NULL, tx - bw / 2.0f, ty - bh / 2.0f, bw, bh, teamColor);
            C2D_DrawCircleSolid(tx, ty - bh / 2.0f - 5.0f * scale, 0.0f, 4.5f * scale, C2D_Color32(185, 145, 110, 255));

            if (aiming && b->team == 1) {
                line(tx - 8, ty - 11, tx + 8, ty - 11, 1.2f, RED);
                line(tx, ty - 18, tx, ty - 2, 1.2f, RED);
            }
        }
    }

    if (!scoped) {
        float spread = aiming ? 3.0f : 7.0f;
        line(200 - spread - 4, 120, 200 - spread, 120, 2.0f, WHITE);
        line(200 + spread, 120, 200 + spread + 4, 120, 2.0f, WHITE);
        line(200, 120 - spread - 4, 200, 120 - spread, 2.0f, WHITE);
        line(200, 120 + spread, 200, 120 + spread + 4, 2.0f, WHITE);
    } else {
        drawScope();
    }

    rect(NULL, 0, 210, 400, 30, C2D_Color32(6, 8, 10, 240));
    char s[96];
    snprintf(s, sizeof(s), "HP %d ARM %d %s %d/%d K:%d", health, armour, weaponNames[weapon], ammo, reserveAmmo, kills);
    drawText(8, 215, 0.52f, WHITE, s);
    snprintf(s, sizeof(s), "$%d", money);
    drawText(330, 215, 0.52f, YELLOW, s);

    rect(NULL, 0, 0, 400, 24, C2D_Color32(8, 10, 12, 225));
    drawText(8, 5, 0.52f, WHITE, "DUST II  |  BARBEQUE MARINE / T SIDE");

    if (roundState == 1) {
        drawText(260, 5, 0.52f, GREEN, "T WIN - START");
    } else if (roundState == 2) {
        drawText(260, 5, 0.52f, RED, "CT WIN - START");
    } else if (planted) {
        drawText(260, 5, 0.52f, YELLOW, "BOMB PLANTED");
    } else {
        snprintf(s, sizeof(s), "ROUND %02d:%02d", roundFrames / 3600, (roundFrames / 60) % 60);
        drawText(270, 5, 0.48f, YELLOW, s);
    }
}

static void drawRadar(void) {
    if (showBuy) {
        drawBuyMenu();
        return;
    }

    rect(NULL, 0, 0, 320, 240, C2D_Color32(20, 25, 29, 255));
    drawText(12, 8, 0.68f, WHITE, "DUST II  -  TACTICAL MAP");

    /* terrain blocks */
    rect(NULL, 20, 46, 100, 52, C2D_Color32(83, 78, 66, 255));
    rect(NULL, 120, 63, 70, 30, C2D_Color32(83, 78, 66, 255));
    rect(NULL, 186, 44, 108, 58, C2D_Color32(83, 78, 66, 255));
    rect(NULL, 186, 142, 108, 54, C2D_Color32(83, 78, 66, 255));
    rect(NULL, 116, 87, 24, 78, C2D_Color32(83, 78, 66, 255));
    rect(NULL, 136, 142, 52, 24, C2D_Color32(83, 78, 66, 255));

    line(68, 72, 155, 78, 5.0f, SAND);
    line(155, 78, 223, 73, 5.0f, SAND);
    line(155, 78, 155, 158, 5.0f, SAND);
    line(155, 158, 220, 170, 5.0f, SAND);

    drawText(220, 58, 0.68f, RED, "A");
    drawText(220, 160, 0.68f, RED, "B");
    drawText(29, 52, 0.42f, WHITE, "T SPAWN");
    drawText(130, 68, 0.42f, WHITE, "MID");
    drawText(142, 180, 0.42f, WHITE, "TUNNEL");

    float px = 72.0f + playerX * 12.0f;
    float py = 73.0f + playerY * 12.0f;
    C2D_DrawCircleSolid(px, py, 0.0f, 5.0f, GREEN);
    line(px, py, px + cosf(yaw) * 14.0f, py + sinf(yaw) * 14.0f, 2.0f, GREEN);

    for (int i = 0; i < BOT_COUNT; ++i) {
        if (!bots[i].alive) {
            continue;
        }
        float bx = 153.0f + bots[i].x * 24.0f;
        float by = 112.0f + bots[i].y * 22.0f;
        C2D_DrawCircleSolid(bx, by, 0.0f, 3.6f, bots[i].team == 1 ? BLUE : RED);
    }

    drawText(12, 192, 0.48f, WHITE, "GREEN YOU  RED T  BLUE CT");
    if (roundState == 1) {
        drawText(12, 208, 0.48f, GREEN, "TERRORISTS WIN - START NEXT ROUND");
    } else if (roundState == 2) {
        drawText(12, 208, 0.48f, RED, "COUNTER-TERRORISTS WIN - START");
    } else {
        drawText(12, 208, 0.48f, YELLOW, planted ? "BOMB PLANTED - PROTECT SITE" : "X BUY MENU  START PLANT AT SITE");
    }
    drawText(12, 222, 0.43f, WHITE, "TOUCH DRAG LOOK | L AIM | R FIRE | A RELOAD");
}

int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    textBuf = C2D_TextBufNew(4096);

    while (aptMainLoop()) {
        hidScanInput();
        u32 held = hidKeysHeld();
        u32 down = hidKeysDown();
        circlePosition cp;
        touchPosition touch;
        hidCircleRead(&cp);
        hidTouchRead(&touch);

        float moveX = cp.dx / 156.0f;
        float moveY = cp.dy / 156.0f;

        if (roundState == 0 && !showBuy) {
            playerX += moveX * 0.055f;
            playerY -= moveY * 0.055f;
            if (playerX < -4.0f) playerX = -4.0f;
            if (playerX > 4.0f) playerX = 4.0f;
            if (playerY < -4.0f) playerY = -4.0f;
            if (playerY > 4.0f) playerY = 4.0f;
        }

        if ((held & KEY_TOUCH) && !showBuy) {
            if (wasTouching) {
                int dx = (int)touch.px - (int)prevTouch.px;
                int dy = (int)touch.py - (int)prevTouch.py;
                yaw += dx * 0.012f;
                pitch += dy * 0.35f;
                if (pitch < -35.0f) pitch = -35.0f;
                if (pitch > 35.0f) pitch = 35.0f;
            }
            prevTouch = touch;
            wasTouching = true;
        } else {
            wasTouching = false;
        }

        if (down & KEY_X) {
            showBuy = !showBuy;
        }

        if (showBuy && (down & KEY_TOUCH)) {
            if (touch.py >= 55 && touch.py < 89) {
                buyAt(0);
            } else if (touch.py >= 94 && touch.py < 128) {
                buyAt(1);
            } else if (touch.py >= 133 && touch.py < 167) {
                buyAt(2);
            } else if (touch.py >= 172 && touch.py < 206) {
                buyAt(3);
            }
        }

        aiming = (held & KEY_L) ? 1 : 0;
        scoped = (aiming && weapon == 1) ? 1 : 0;

        if (roundState == 0 && !showBuy && (down & KEY_R) && fireCooldown == 0 && ammo > 0) {
            ammo--;
            fireCooldown = weapon == 1 ? 22 : 8;

            int best = -1;
            float bestDist = 999.0f;

            for (int i = 0; i < BOT_COUNT; ++i) {
                Bot *b = &bots[i];
                if (!b->alive || b->team != 1) {
                    continue;
                }
                float dx = fabsf(b->x - playerX - yaw * 0.20f);
                float dy = fabsf(b->y - playerY - pitch * 0.01f);
                float limit = weapon == 1 ? 0.22f : (aiming ? 0.25f : 0.48f);
                if (dx < limit && dy < (aiming ? 0.6f : 1.1f) && dx + dy < bestDist) {
                    best = i;
                    bestDist = dx + dy;
                }
            }

            if (best >= 0) {
                bots[best].hp -= weapon == 1 ? 100 : (weapon == 0 ? 34 : 20);
                if (bots[best].hp <= 0) {
                    bots[best].alive = 0;
                    kills++;
                    money += 300;
                    if (aliveEnemies() == 0) {
                        roundState = 1;
                    }
                }
            }
        }

        if (fireCooldown > 0) {
            fireCooldown--;
        }

        if (down & KEY_A) {
            int mag = weapon == 1 ? 10 : (weapon == 0 ? 30 : 20);
            int n = mag - ammo;
            if (n > reserveAmmo) {
                n = reserveAmmo;
            }
            if (n > 0) {
                ammo += n;
                reserveAmmo -= n;
            }
        }

        if ((down & KEY_Y) && roundState == 0) {
            weapon = (weapon + 1) % 3;
            ammo = weapon == 1 ? 10 : (weapon == 0 ? 30 : 20);
            reserveAmmo = weapon == 1 ? 30 : 90;
        }

        if (roundState == 0) {
            if (roundFrames > 0) {
                roundFrames--;
            }

            if (!planted && (down & KEY_START)) {
                float da = fabsf(playerX - 2.2f) + fabsf(playerY - 1.8f);
                float db = fabsf(playerX + 2.2f) + fabsf(playerY - 1.8f);
                if (da < 1.1f || db < 1.1f) {
                    planted = 1;
                    bombFrames = 1800;
                }
            }

            if (planted && bombFrames > 0) {
                bombFrames--;
                if (bombFrames == 0) {
                    roundState = 1;
                }
            }

            if (!planted && roundFrames == 0) {
                roundState = 2;
            }
            if (health <= 0) {
                roundState = 2;
            }

            if (++botThink >= 45) {
                botThink = 0;
                for (int i = 0; i < BOT_COUNT; ++i) {
                    Bot *b = &bots[i];
                    if (!b->alive || b->team != 1) {
                        continue;
                    }
                    float dx = playerX - b->x;
                    float dy = playerY - b->y;
                    float dist = fabsf(dx) + fabsf(dy);
                    if (dist > 1.0f) {
                        b->x += (dx / (dist + 0.01f)) * 0.12f;
                        b->y += (dy / (dist + 0.01f)) * 0.12f;
                    } else {
                        int damage = 7;
                        if (armour > 0) {
                            int absorbed = damage / 2;
                            if (absorbed > armour) {
                                absorbed = armour;
                            }
                            armour -= absorbed;
                            damage -= absorbed;
                        }
                        health -= damage;
                        if (health < 0) {
                            health = 0;
                        }
                    }
                }
            }
        } else if (down & KEY_START) {
            resetRound();
        }

        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_SceneBegin(top);
        drawWorld();
        C2D_SceneBegin(bottom);
        drawRadar();
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(textBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
