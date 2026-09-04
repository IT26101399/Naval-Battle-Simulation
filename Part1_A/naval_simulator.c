/*
 * ============================================================
 *  Advanced Naval Battle Simulator - Part 1-A
 *  SE1012: Programming Methodology
 *  Sri Lanka Institute of Information Technology
 * ============================================================
 *
 *  A stationary Battleship (B) engages Escort ships (E) on a
 *  2D square canvas. Shells follow parabolic projectile motion.
 *
 *  Part 1-A assumptions:
 *    - B reloads and fires in 0 seconds (instant)
 *    - Each E ship can fire only once
 *    - A single shell impact destroys any ship
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

/* ======================== Constants ======================== */

#define MAX_ESCORTS    100
#define GRAVITY        9.81
#define PI             3.14159265358979323846

#define DEG_TO_RAD(d)  ((d) * PI / 180.0)
#define RAD_TO_DEG(r)  ((r) * 180.0 / PI)

/* ===================== Enumerations ======================= */

typedef enum {
    EA, EB, EC, ED, EE,
    NUM_ESCORT_TYPES
} EscortType;

typedef enum {
    TYPE_U, TYPE_M, TYPE_R, TYPE_S,
    NUM_BATTLESHIP_TYPES
} BattleshipType;

/* ====================== Structures ======================== */

/* Static reference data for each escort ship type */
typedef struct {
    char notation[4];       /* "EA" .. "EE"                    */
    char className[40];     /* e.g. "1936A-class Destroyer"    */
    char gunName[50];       /* e.g. "SK C/34 naval gun"        */
    double impactPower;     /* fractional damage per hit       */
    double angleRange;      /* theta_H - theta_L  (degrees)    */
} EscortTypeInfo;

/* Static reference data for each battleship type */
typedef struct {
    char name[50];          /* e.g. "USS Iowa (BB-61)"         */
    char notation;          /* 'U', 'M', 'R', 'S'             */
    char gunName[50];       /* e.g. "50-caliber Mark 7 gun"    */
} BattleshipTypeInfo;

/* An individual escort ship on the battlefield */
typedef struct {
    int    index;           /* unique identifier (1-based)     */
    EscortType type;
    double x, y;            /* position on canvas              */
    double minVelocity;     /* shell velocity lower bound      */
    double maxVelocity;     /* shell velocity upper bound      */
    double minAngle;        /* theta_L (degrees)               */
    double maxAngle;        /* theta_H (degrees)               */
    double impactPower;     /* damage fraction per hit          */
    double maxRange;        /* maximum horizontal attack range  */
    int    alive;           /* 1 = operational, 0 = destroyed  */
} EscortShip;

/* The player's battleship */
typedef struct {
    BattleshipType type;
    double x, y;            /* position on canvas              */
    double maxVelocity;     /* maximum shell velocity          */
    double maxRange;        /* maximum horizontal attack range  */
    int    alive;           /* 1 = operational, 0 = destroyed  */
} Battleship;

/* Record of a single successful hit on an escort ship */
typedef struct {
    int    escortIndex;     /* which E was hit                 */
    double distance;        /* distance from B to E (metres)   */
    double flightTime;      /* shell time-of-flight (seconds)  */
    double angleUsed;       /* launch angle (degrees)          */
    double velocityUsed;    /* launch velocity (m/s)           */
} HitRecord;

/* Aggregated simulation results */
typedef struct {
    int    battleshipSunk;       /* 1 if B was destroyed        */
    int    sunkByEscortIndex;    /* index of E that sank B      */
    int    escortsHitCount;      /* number of E ships destroyed */
    HitRecord hits[MAX_ESCORTS]; /* details of each hit         */
    double battleDuration;       /* time until last shell lands  */
} SimResult;

/* ================ Global Type-Data Tables ================= */

const EscortTypeInfo ESCORT_INFO[NUM_ESCORT_TYPES] = {
    { "EA", "1936A-class Destroyer",   "SK C/34 naval gun",        0.08, 20.0 },
    { "EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun",    0.06, 30.0 },
    { "EC", "Matsu-class Destroyer",   "Type 89 dual-purpose gun", 0.07, 25.0 },
    { "ED", "F-class Escort Ships",    "SK C/32 naval gun",        0.05, 50.0 },
    { "EE", "Japanese Kaibokan",       "(4.7 inch) naval guns",    0.04, 70.0 }
};

const BattleshipTypeInfo BATTLESHIP_INFO[NUM_BATTLESHIP_TYPES] = {
    { "USS Iowa (BB-61)",      'U', "50-caliber Mark 7 gun"   },
    { "MS King George V",      'M', "(356 mm) Mark VII gun"   },
    { "Richelieu",             'R', "(15 inch) Mle 1935 gun"  },
    { "Sovetsky Soyuz-class",  'S', "(16 inch) B-37 gun"      }
};

/* ================= Utility Functions ====================== */

/**
 * Generate a random double in [min, max].
 */
double randomDouble(double min, double max) {
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

/**
 * Generate a random integer in [min, max] (inclusive).
 */
int randomInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

/**
 * Clear the stdin input buffer.
 */
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ================== Physics Functions ===================== */

/**
 * Horizontal range for a shell fired at 'velocity' and 'angleDeg'.
 *   R = V^2 * sin(2 theta) / g
 */
double calculateRange(double velocity, double angleDeg) {
    double angleRad = DEG_TO_RAD(angleDeg);
    return (velocity * velocity * sin(2.0 * angleRad)) / GRAVITY;
}

/**
 * Total flight time of a shell.
 *   T = 2 V sin(theta) / g
 */
double calculateFlightTime(double velocity, double angleDeg) {
    double angleRad = DEG_TO_RAD(angleDeg);
    return (2.0 * velocity * sin(angleRad)) / GRAVITY;
}

/**
 * Maximum range a ship can achieve given its velocity and angle limits.
 * sin(2 theta) is maximised at theta = 45 deg.  If 45 deg is outside the
 * allowed range, the boundary closest to 45 deg is used instead.
 */
double calculateMaxRange(double maxVelocity, double minAngleDeg, double maxAngleDeg) {
    double bestAngle;

    if (minAngleDeg <= 45.0 && maxAngleDeg >= 45.0) {
        bestAngle = 45.0;
    } else if (maxAngleDeg < 45.0) {
        bestAngle = maxAngleDeg;       /* closest to 45 from below */
    } else {
        bestAngle = minAngleDeg;       /* closest to 45 from above */
    }

    return calculateRange(maxVelocity, bestAngle);
}

/**
 * Euclidean distance between two 2-D points.
 */
double calculateDistance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

/**
 * Compute launch angle and velocity needed to hit a target at 'distance'.
 * Uses maximum velocity with the lowest trajectory for shortest flight time.
 *
 * Returns 1 on success, 0 if the target is out of range.
 */
int calculateFiringParams(double distance, double maxVelocity,
                           double minAngleDeg, double maxAngleDeg,
                           double *outAngle, double *outVelocity) {

    double maxRange = calculateMaxRange(maxVelocity, minAngleDeg, maxAngleDeg);

    if (distance > maxRange + 0.001)
        return 0;   /* out of range */

    if (distance < 0.001) {
        /* target is essentially at our position */
        *outAngle    = minAngleDeg;
        *outVelocity = 0.0;
        return 1;
    }

    /* sin(2 theta) needed when firing at V_max */
    double sinVal = (distance * GRAVITY) / (maxVelocity * maxVelocity);
    if (sinVal > 1.0) sinVal = 1.0;

    /* Low-angle solution gives shorter flight time */
    double angle = RAD_TO_DEG(asin(sinVal)) / 2.0;

    /* Clamp to the ship's allowed angle band */
    if (angle < minAngleDeg) angle = minAngleDeg;
    if (angle > maxAngleDeg) angle = maxAngleDeg;

    /* Velocity required at the (possibly clamped) angle */
    double sinTwoTheta = sin(2.0 * DEG_TO_RAD(angle));
    if (sinTwoTheta < 0.0001) return 0;

    double velocityNeeded = sqrt((distance * GRAVITY) / sinTwoTheta);
    if (velocityNeeded > maxVelocity + 0.001)
        return 0;

    *outAngle    = angle;
    *outVelocity = velocityNeeded;
    return 1;
}

/* =============== Initialisation Functions ================== */

/**
 * Prompt the user to select a battleship type.
 */
BattleshipType getBattleshipType(void) {
    char choice;

    printf("\n  Select Battleship Type:\n");
    printf("  -----------------------------------------------\n");
    for (int i = 0; i < NUM_BATTLESHIP_TYPES; i++) {
        printf("  [%c]  %s  (%s)\n",
               BATTLESHIP_INFO[i].notation,
               BATTLESHIP_INFO[i].name,
               BATTLESHIP_INFO[i].gunName);
    }
    printf("\n  Enter choice (U/M/R/S): ");
    scanf(" %c", &choice);

    switch (choice) {
        case 'U': case 'u': return TYPE_U;
        case 'M': case 'm': return TYPE_M;
        case 'R': case 'r': return TYPE_R;
        case 'S': case 's': return TYPE_S;
        default:
            printf("  Invalid choice. Defaulting to USS Iowa (U).\n");
            return TYPE_U;
    }
}

/**
 * Initialise the battleship with user-provided or random values.
 */
void initBattleship(Battleship *b, double canvasSize) {
    int posChoice;

    b->type = getBattleshipType();

    printf("\n  Battleship Position:\n");
    printf("  [1] Enter coordinates manually\n");
    printf("  [2] Generate randomly\n");
    printf("  Choice: ");
    scanf("%d", &posChoice);

    if (posChoice == 1) {
        printf("  Enter X coordinate (0 - %.0f): ", canvasSize);
        scanf("%lf", &b->x);
        printf("  Enter Y coordinate (0 - %.0f): ", canvasSize);
        scanf("%lf", &b->y);

        /* Clamp to canvas bounds */
        if (b->x < 0) b->x = 0;
        if (b->x > canvasSize) b->x = canvasSize;
        if (b->y < 0) b->y = 0;
        if (b->y > canvasSize) b->y = canvasSize;
    } else {
        b->x = randomDouble(0, canvasSize);
        b->y = randomDouble(0, canvasSize);
    }

    printf("  Enter maximum shell velocity for Battleship (m/s): ");
    scanf("%lf", &b->maxVelocity);

    /* B can fire at any angle 0-90 deg, so max range occurs at 45 deg */
    b->maxRange = (b->maxVelocity * b->maxVelocity) / GRAVITY;
    b->alive = 1;
}

/**
 * Initialise N escort ships with random positions, types, and attributes.
 */
void initEscortShips(EscortShip ships[], int count,
                      double canvasSize, double bMaxVelocity) {

    for (int i = 0; i < count; i++) {
        ships[i].index = i + 1;
        ships[i].type  = (EscortType)randomInt(0, NUM_ESCORT_TYPES - 1);
        ships[i].x     = randomDouble(0, canvasSize);
        ships[i].y     = randomDouble(0, canvasSize);
        ships[i].impactPower = ESCORT_INFO[ships[i].type].impactPower;
        ships[i].alive  = 1;

        /* ---------- Angle range ---------- */
        double angleRange  = ESCORT_INFO[ships[i].type].angleRange;
        double maxMinAngle = 90.0 - angleRange;
        if (maxMinAngle < 0) maxMinAngle = 0;

        ships[i].minAngle = randomDouble(0, maxMinAngle);
        ships[i].maxAngle = ships[i].minAngle + angleRange;

        /* ---------- Velocities ---------- */
        if (ships[i].type == EA) {
            /* EA's max velocity is fixed at 1.2 * V_B_max */
            ships[i].maxVelocity = 1.2 * bMaxVelocity;
        } else {
            /* All other types: max velocity < V_B_max */
            ships[i].maxVelocity = randomDouble(0.3 * bMaxVelocity,
                                                 0.95 * bMaxVelocity);
        }
        ships[i].minVelocity = randomDouble(0.1 * ships[i].maxVelocity,
                                              0.5 * ships[i].maxVelocity);

        /* ---------- Maximum attack range ---------- */
        ships[i].maxRange = calculateMaxRange(ships[i].maxVelocity,
                                               ships[i].minAngle,
                                               ships[i].maxAngle);
    }
}

/* ================ Simulation Functions ==================== */

/**
 * Run the Part 1-A simulation.
 *
 * Logic (simultaneous engagement at t = 0):
 *   Since B's reload time is 0 seconds, all firing happens at
 *   the same instant.  Both sides resolve simultaneously:
 *
 *   1.  B fires at every E ship within its attack range,
 *       destroying them all.
 *   2.  Every E ship whose attack range covers B also fires
 *       at B.  Since a single hit sinks B, the *first* such
 *       E ship (by index order) is the one that sank B.
 *   3.  Both outcomes coexist — B can destroy E ships AND
 *       be sunk in the same engagement.
 *   4.  Battle duration = max shell flight time among B's shots.
 */
SimResult runSimulation1A(Battleship *b, EscortShip escorts[], int numEscorts) {
    SimResult result;
    memset(&result, 0, sizeof(SimResult));
    result.sunkByEscortIndex = -1;

    double maxFlightTime = 0.0;

    /* --- B fires at all E ships in its range (simultaneous) --- */
    for (int i = 0; i < numEscorts; i++) {
        if (!escorts[i].alive) continue;

        double dist = calculateDistance(b->x, b->y,
                                         escorts[i].x, escorts[i].y);

        if (dist <= b->maxRange) {
            double angle, velocity;

            if (calculateFiringParams(dist, b->maxVelocity,
                                       0.0, 90.0,
                                       &angle, &velocity)) {
                double ft = calculateFlightTime(velocity, angle);

                HitRecord *hr = &result.hits[result.escortsHitCount];
                hr->escortIndex  = escorts[i].index;
                hr->distance     = dist;
                hr->flightTime   = ft;
                hr->angleUsed    = angle;
                hr->velocityUsed = velocity;

                result.escortsHitCount++;
                escorts[i].alive = 0;

                if (ft > maxFlightTime)
                    maxFlightTime = ft;
            }
        }
    }

    result.battleDuration = maxFlightTime;

    /* --- E ships fire at B (simultaneous — uses original positions) --- */
    /* Note: Even E ships destroyed by B in this round had already fired  */
    /* their shells at t = 0, so we check ALL original E ships.           */
    for (int i = 0; i < numEscorts; i++) {
        /* Check every ship's original range, regardless of alive status, */
        /* because all shells were launched simultaneously at t = 0.      */
        double dist = calculateDistance(escorts[i].x, escorts[i].y,
                                         b->x, b->y);

        if (dist <= escorts[i].maxRange) {
            result.battleshipSunk    = 1;
            result.sunkByEscortIndex = escorts[i].index;
            b->alive = 0;
            break;   /* first E in range sinks B */
        }
    }

    return result;
}

/* ================== File I/O Functions ==================== */

/**
 * Save the initial battlefield conditions to a text file.
 */
void saveInitialConditions(const char *filename,
                            Battleship *b, EscortShip escorts[],
                            int numEscorts, double canvasSize,
                            unsigned int seed) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("  Error: Could not create file '%s'.\n", filename);
        return;
    }

    fprintf(fp, "========================================\n");
    fprintf(fp, "   INITIAL BATTLEFIELD CONDITIONS\n");
    fprintf(fp, "========================================\n\n");

    fprintf(fp, "Canvas Size       : %.2f x %.2f\n", canvasSize, canvasSize);
    fprintf(fp, "Random Seed       : %u\n\n", seed);

    /* Battleship */
    fprintf(fp, "--- Battleship ---\n");
    fprintf(fp, "Type              : %c (%s)\n",
            BATTLESHIP_INFO[b->type].notation,
            BATTLESHIP_INFO[b->type].name);
    fprintf(fp, "Gun               : %s\n",
            BATTLESHIP_INFO[b->type].gunName);
    fprintf(fp, "Position          : (%.2f, %.2f)\n", b->x, b->y);
    fprintf(fp, "Max Shell Velocity: %.2f m/s\n", b->maxVelocity);
    fprintf(fp, "Max Attack Range  : %.2f m\n\n", b->maxRange);

    /* Escort ships */
    fprintf(fp, "--- Escort Ships (%d total) ---\n\n", numEscorts);

    for (int i = 0; i < numEscorts; i++) {
        fprintf(fp, "  Escort #%d\n", escorts[i].index);
        fprintf(fp, "    Type           : %s (%s)\n",
                ESCORT_INFO[escorts[i].type].notation,
                ESCORT_INFO[escorts[i].type].className);
        fprintf(fp, "    Gun            : %s\n",
                ESCORT_INFO[escorts[i].type].gunName);
        fprintf(fp, "    Position       : (%.2f, %.2f)\n",
                escorts[i].x, escorts[i].y);
        fprintf(fp, "    Velocity Range : %.2f - %.2f m/s\n",
                escorts[i].minVelocity, escorts[i].maxVelocity);
        fprintf(fp, "    Angle Range    : %.2f - %.2f degrees\n",
                escorts[i].minAngle, escorts[i].maxAngle);
        fprintf(fp, "    Impact Power   : %.2f\n",
                escorts[i].impactPower);
        fprintf(fp, "    Max Attack Range: %.2f m\n\n",
                escorts[i].maxRange);
    }

    fclose(fp);
    printf("  [FILE] Initial conditions saved to '%s'\n", filename);
}

/**
 * Save simulation results and final battlefield state to a text file.
 */
void saveSimulationResults(const char *filename,
                            Battleship *b, EscortShip escorts[],
                            int numEscorts, SimResult *result) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("  Error: Could not create file '%s'.\n", filename);
        return;
    }

    fprintf(fp, "========================================\n");
    fprintf(fp, "   SIMULATION RESULTS  -  PART 1-A\n");
    fprintf(fp, "========================================\n\n");

    if (result->battleshipSunk) {
        fprintf(fp, "OUTCOME           : Battleship SUNK\n");
        fprintf(fp, "Sunk by Escort    : #%d\n", result->sunkByEscortIndex);
    } else {
        fprintf(fp, "OUTCOME           : Battleship SURVIVED\n");
    }

    fprintf(fp, "Escorts Destroyed : %d / %d\n",
            result->escortsHitCount, numEscorts);
    fprintf(fp, "Battle Duration   : %.4f seconds\n\n",
            result->battleDuration);

    if (result->escortsHitCount > 0) {
        fprintf(fp, "--- Escort Ships Destroyed by Battleship ---\n\n");
        for (int i = 0; i < result->escortsHitCount; i++) {
            HitRecord *hr = &result->hits[i];
            fprintf(fp, "  Hit #%d\n", i + 1);
            fprintf(fp, "    Escort Index   : #%d\n", hr->escortIndex);
            fprintf(fp, "    Distance       : %.2f m\n", hr->distance);
            fprintf(fp, "    Shell Angle    : %.2f degrees\n", hr->angleUsed);
            fprintf(fp, "    Shell Velocity : %.2f m/s\n", hr->velocityUsed);
            fprintf(fp, "    Time to Hit    : %.4f seconds\n\n", hr->flightTime);
        }
    }

    /* Final state of every ship */
    fprintf(fp, "--- Final Battlefield State ---\n\n");
    fprintf(fp, "Battleship : %s\n\n", b->alive ? "OPERATIONAL" : "DESTROYED");
    fprintf(fp, "Escort Ships:\n");

    for (int i = 0; i < numEscorts; i++) {
        fprintf(fp, "  #%-3d [%s]  %s\n",
                escorts[i].index,
                ESCORT_INFO[escorts[i].type].notation,
                escorts[i].alive ? "OPERATIONAL" : "DESTROYED");
    }

    fclose(fp);
    printf("  [FILE] Simulation results saved to '%s'\n", filename);
}

/* ================== Display Functions ===================== */

/**
 * Print simulation results to the console.
 */
void displayResults(SimResult *result, int numEscorts) {

    printf("\n  +==========================================+\n");
    printf("  |      SIMULATION RESULTS (Part 1-A)      |\n");
    printf("  +==========================================+\n\n");

    if (result->battleshipSunk) {
        printf("  >> BATTLESHIP SUNK!\n");
        printf("     Destroyed by Escort Ship #%d\n",
               result->sunkByEscortIndex);
    } else {
        printf("  >> BATTLESHIP SURVIVED!\n");
    }

    printf("     Escort ships destroyed : %d / %d\n",
           result->escortsHitCount, numEscorts);
    printf("     Battle duration        : %.4f seconds\n",
           result->battleDuration);

    if (result->escortsHitCount > 0) {
        printf("\n  Escorts Destroyed by Battleship:\n");
        printf("  %-8s %-14s %-14s %-14s %-14s\n",
               "Index", "Distance(m)", "Time(s)",
               "Angle(deg)", "Velocity(m/s)");
        printf("  -----------------------------------------------------------"
               "-----------\n");
        for (int i = 0; i < result->escortsHitCount; i++) {
            HitRecord *hr = &result->hits[i];
            printf("  #%-7d %-14.2f %-14.4f %-14.2f %-14.2f\n",
                   hr->escortIndex, hr->distance,
                   hr->flightTime, hr->angleUsed,
                   hr->velocityUsed);
        }
    }
    printf("\n");
}

/**
 * Print a summary of the initial battlefield layout.
 */
void displayBattlefield(Battleship *b, EscortShip escorts[],
                          int numEscorts, double canvasSize) {

    printf("\n  +==========================================+\n");
    printf("  |          BATTLEFIELD OVERVIEW            |\n");
    printf("  +==========================================+\n\n");

    printf("  Canvas : %.0f x %.0f\n\n", canvasSize, canvasSize);

    printf("  BATTLESHIP [%c] - %s\n",
           BATTLESHIP_INFO[b->type].notation,
           BATTLESHIP_INFO[b->type].name);
    printf("  Position : (%.2f, %.2f)  |  Max V : %.2f m/s"
           "  |  Range : %.2f m\n\n",
           b->x, b->y, b->maxVelocity, b->maxRange);

    printf("  ESCORT SHIPS (%d):\n", numEscorts);
    printf("  %-6s %-6s %-22s %-14s %-10s\n",
           "Index", "Type", "Position", "Max Range(m)", "Status");
    printf("  -----------------------------------------------------------"
           "---\n");

    for (int i = 0; i < numEscorts; i++) {
        printf("  #%-5d %-6s (%-8.1f, %-8.1f)   %-14.2f %-10s\n",
               escorts[i].index,
               ESCORT_INFO[escorts[i].type].notation,
               escorts[i].x, escorts[i].y,
               escorts[i].maxRange,
               escorts[i].alive ? "ACTIVE" : "DESTROYED");
    }
    printf("\n");
}

/**
 * Show the help / instructions screen.
 */
void displayInstructions(void) {

    printf("\n  +====================================================+\n");
    printf("  |             SIMULATOR INSTRUCTIONS                 |\n");
    printf("  +====================================================+\n\n");

    printf("  This is an Advanced Naval Battle Simulator for navy\n");
    printf("  training. A Battleship (B) engages multiple Escort\n");
    printf("  ships (E) on a 2D square canvas.\n\n");

    printf("  SETUP\n");
    printf("  -----\n");
    printf("  1. Choose your Battleship type (U, M, R, or S).\n");
    printf("  2. Set the canvas size (D x D) and number of escorts.\n");
    printf("  3. Enter or randomly generate B's position & velocity.\n");
    printf("  4. E ships are randomly placed and assigned types.\n\n");

    printf("  PHYSICS\n");
    printf("  -------\n");
    printf("  Shells follow parabolic projectile motion:\n");
    printf("    Range       = V^2 * sin(2 * theta) / g\n");
    printf("    Flight time = 2 * V * sin(theta) / g\n");
    printf("    g = 9.81 m/s^2\n\n");

    printf("  ATTACK RANGE\n");
    printf("  ------------\n");
    printf("  B can fire at any angle (0-90 deg), so its max range\n");
    printf("  = V_max^2 / g  (achieved at 45 deg).\n\n");
    printf("  E ships have restricted angle bands, so their max\n");
    printf("  range uses the best angle within that band.\n\n");

    printf("  OBJECTIVE\n");
    printf("  ---------\n");
    printf("  Destroy as many escort ships as possible while\n");
    printf("  keeping the battleship alive.\n\n");

    printf("  Press Enter to return to the main menu...");
    clearInputBuffer();
    getchar();
}

/**
 * Load and display saved simulation statistics from text files.
 */
void displayStatistics(void) {
    FILE *fp;
    char line[256];

    printf("\n  +==========================================+\n");
    printf("  |        SIMULATION STATISTICS             |\n");
    printf("  +==========================================+\n\n");

    /* Initial conditions */
    printf("  --- Initial Conditions ---\n\n");
    fp = fopen("initial_conditions_1A.txt", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp))
            printf("  %s", line);
        fclose(fp);
    } else {
        printf("  No initial conditions file found.\n");
        printf("  Run a simulation first.\n");
    }

    /* Results */
    printf("\n\n  --- Simulation Results ---\n\n");
    fp = fopen("simulation_results_1A.txt", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp))
            printf("  %s", line);
        fclose(fp);
    } else {
        printf("  No simulation results file found.\n");
        printf("  Run a simulation first.\n");
    }

    printf("\n  Press Enter to return to the main menu...");
    clearInputBuffer();
    getchar();
}

/* =================== Menu Functions ======================= */

/**
 * Display the main menu and return the user's choice.
 */
int showMainMenu(void) {
    int choice;

    printf("\n  +==========================================+\n");
    printf("  |    ADVANCED NAVAL BATTLE SIMULATOR       |\n");
    printf("  |              Part 1-A                    |\n");
    printf("  +==========================================+\n\n");
    printf("  1. Start Simulation\n");
    printf("  2. View Instructions\n");
    printf("  3. Simulation Statistics\n");
    printf("  4. Exit\n\n");
    printf("  Enter choice: ");

    if (scanf("%d", &choice) != 1) {
        clearInputBuffer();
        return -1;
    }
    return choice;
}

/* ===================== Main =============================== */

int main(void) {
    Battleship  battleship;
    EscortShip  escorts[MAX_ESCORTS];
    int         numEscorts  = 0;
    double      canvasSize  = 0;
    unsigned int seed       = 0;
    int         running     = 1;

    while (running) {
        int choice = showMainMenu();

        switch (choice) {

        /* ---------- 1. Start Simulation ---------- */
        case 1: {
            printf("\n  ============ SIMULATION SETUP ============\n");

            /* Random seed */
            printf("\n  Enter random seed (0 for time-based): ");
            scanf("%u", &seed);
            if (seed == 0)
                seed = (unsigned int)time(NULL);
            srand(seed);
            printf("  Seed set to: %u\n", seed);

            /* Canvas */
            printf("\n  Enter canvas size D (e.g. 5000): ");
            scanf("%lf", &canvasSize);
            if (canvasSize <= 0) {
                printf("  Invalid size. Using 5000.\n");
                canvasSize = 5000;
            }

            /* Escort count */
            printf("  Enter number of escort ships (1-%d): ", MAX_ESCORTS);
            scanf("%d", &numEscorts);
            if (numEscorts < 1)  numEscorts = 1;
            if (numEscorts > MAX_ESCORTS) numEscorts = MAX_ESCORTS;

            /* Initialise ships */
            initBattleship(&battleship, canvasSize);
            initEscortShips(escorts, numEscorts, canvasSize,
                            battleship.maxVelocity);

            /* Display battlefield */
            displayBattlefield(&battleship, escorts, numEscorts, canvasSize);

            /* Save initial conditions */
            saveInitialConditions("initial_conditions_1A.txt",
                                  &battleship, escorts,
                                  numEscorts, canvasSize, seed);

            /* Run simulation */
            printf("  Running Part 1-A simulation...\n");
            SimResult result = runSimulation1A(&battleship, escorts,
                                               numEscorts);

            /* Show & save results */
            displayResults(&result, numEscorts);
            saveSimulationResults("simulation_results_1A.txt",
                                   &battleship, escorts,
                                   numEscorts, &result);
            break;
        }

        /* ---------- 2. Instructions ---------- */
        case 2:
            displayInstructions();
            break;

        /* ---------- 3. Statistics ---------- */
        case 3:
            displayStatistics();
            break;

        /* ---------- 4. Exit ---------- */
        case 4: {
            char confirm;
            printf("  Are you sure you want to exit? (y/n): ");
            scanf(" %c", &confirm);
            if (confirm == 'y' || confirm == 'Y') {
                printf("\n  Thank you for using the Naval Battle Simulator!\n\n");
                running = 0;
            }
            break;
        }

        default:
            printf("  Invalid choice. Please enter 1-4.\n");
            break;
        }
    }

    return 0;
}
