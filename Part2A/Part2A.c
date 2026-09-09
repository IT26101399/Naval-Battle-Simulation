#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// =============== CONSTANTS ===============
#define MAX_ESCORTS 100
#define MAX_POINTS 100 
#define GRAV 9.81 
#define PI 3.14159265358979323846

// ============== MACROS ===================
#define DEG_TO_RAD(d) ((d) * PI / 180.0)
#define RAD_TO_DEG(r) ((r) * 180.0 / PI)

// ============= STRUCTURES ================
typedef struct {
    char typeNotation[4]; 
    char className[40];
    char gunName[50];
    double impactpower;
    double angleRange;
    double minAngle; 
    double maxAngle; 
    int minBulletVelo;
    int maxBulletVelo;
} TypeInfo_E;

typedef struct {
    char name[50];
    char typeNotation;
    char gunName[50];
} TypeInfo_B;

typedef struct {
    int index;
    TypeInfo_E typeInfo;
    int x, y;    
    int alive;   
} Escortship;

typedef struct {
    int x, y;
} Point;

typedef struct {
    int escortIndex;
    double distance;
    double flightTime;
    double angleUsed;
    double velocityUsed;
    double fireTime; // Part 2-A: Time B pulled the trigger
} HitRecord;

// Attack Target structure for Part 2-A Strategy
typedef struct {
    int e_index; 
    double dist;
    double ft_B_to_E; 
    double ft_E_to_B; 
    double impact;
    double threat_score;
    double b_angle;
    double b_velocity;
} AttackTarget;

typedef struct {
    int e_idx; 
    double t_hit_B; 
    double impact; 
    int canceled; 
} E_Attack;

typedef struct {
    int battleshipSunk;          
    int sunkByEscortIndex;       
    int escortHitCount;          
    HitRecord hits[MAX_ESCORTS]; 
    double battleDuration;
    int attackOrder[MAX_ESCORTS]; // Store ordered indices for output
    int attackOrderCount;
} SimResult;

// ============= GLOBAL VARIABLES ============
int canvasSize = 5000;
int vMax;
int noEscort = 1;
Escortship E_ships[MAX_ESCORTS];
Escortship original_E_ships[MAX_ESCORTS]; 

int selectedBType = 0;    

// Part 1-B/C & 2-A specific variables
int k_iterations = 5; 
int t_jam = 2;        
double theta_min_B = 20.0;
double reloadTimeB = 2.0; // Part 2-A: Time between shots (T_B^q)
Point b_path[MAX_POINTS];

TypeInfo_E info_ETypes[5] = {
    {"EA", "1936A-class Destroyer",   "SK C/34 naval gun",        0.08, 20.0, 0.0, 0.0, 0, 0},
    {"EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun",    0.06, 30.0, 0.0, 0.0, 0, 0},
    {"EC", "Matsu-class Destroyer",   "Type 89 dual-purpose gun", 0.07, 25.0, 0.0, 0.0, 0, 0},
    {"ED", "F-class Escort Ships",    "SK c/32 naval gun",        0.05, 50.0, 0.0, 0.0, 0, 0},
    {"EE", "Japanese Kaibokan",       "(4.7 inch) naval guns",    0.04, 70.0, 0.0, 0.0, 0, 0}
};

int num_ETypes = sizeof(info_ETypes) / sizeof(info_ETypes[0]);

const TypeInfo_B info_BTypes[4] = {
    {"USS Iowa (BB-61)",     'U', "50-caliber Mark 7 gun"},
    {"MS King George V",     'M', "(356 mm) Mark VII gun"},
    {"Richelieu",            'R', "(15 inch) Mle 1935 gun"},
    {"Sovetsky Soyuz-class", 'S', "(16 inch) B-37 gun"}
};

int num_BTypes = sizeof(info_BTypes) / sizeof(info_BTypes[0]);

// ============= UTILITY FUNCTIONS ===================

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void clearLastLine() {
    printf("\033[1A\033[2K\r");
}

int genRanIntBetween(int min, int max) {
    return min + rand() % (max - min + 1);
}

double genRanDoubleBetween(double min, double max) {
    double scale = (double)rand() / (double)RAND_MAX;
    return min + scale * (max - min);
}

// ============= CORE LOGIC FUNCTIONS ===================

void addETypeValues(int vMax) {
    for (int i = 0; i < num_ETypes; i++) {
        double maximumPossibleMaxAngle = 90.00 - info_ETypes[i].angleRange;
        info_ETypes[i].minAngle = genRanDoubleBetween(0, maximumPossibleMaxAngle);
        info_ETypes[i].maxAngle = info_ETypes[i].minAngle + info_ETypes[i].angleRange;
        info_ETypes[i].maxBulletVelo = genRanIntBetween(1, vMax);
        info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo - 1);

        if (strcmp(info_ETypes[i].typeNotation, "EA") == 0) {
            info_ETypes[i].maxBulletVelo = (int)(1.2 * vMax);
            info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo);
        }
    }
}

void generateEInstances() {
    for (int i = 0; i < noEscort; i++) {
        E_ships[i] = (Escortship){ i + 1, info_ETypes[genRanIntBetween(0, num_ETypes - 1)], genRanIntBetween(0, canvasSize), genRanIntBetween(0, canvasSize), 1 };
        original_E_ships[i] = E_ships[i]; 
    }
}

void generateBPath() {
    for (int i = 0; i < k_iterations; i++) {
        b_path[i].x = genRanIntBetween(0, canvasSize);
        b_path[i].y = genRanIntBetween(0, canvasSize);
    }
}

void resetBattlefield() {
    for (int i = 0; i < noEscort; i++) {
        E_ships[i] = original_E_ships[i];
    }
}

// ============= UI & INPUT FUNCTIONS ===================

void printHeader() {
    printf("======================================\n");
    printf("|        BATTLESHIP SIMULATOR        |\n");
    printf("|          Part 2 - A          |\n");
    printf("======================================\n\n");
}

void configureArena() {
    while (true) {
        printf("Enter canvas size (100 - 10000): ");
        if (scanf("%d", &canvasSize) == 1 && canvasSize >= 100 && canvasSize <= 10000) break;
        clearLastLine(); clearInputBuffer();
    }
    while (true) {
        printf("Enter no. of Escortships (1 - %d): ", MAX_ESCORTS);
        if (scanf("%d", &noEscort) == 1 && noEscort <= MAX_ESCORTS && noEscort >= 1) break;
        clearLastLine(); clearInputBuffer();
    }
}

void configureBattleship() {
    printf("\n");
    for (int i = 0; i < num_BTypes; i++) {
        printf("%d. %c - %s\n", (i + 1), info_BTypes[i].typeNotation, info_BTypes[i].name);
    }
    printf("\n");
    while (true) {
        int temp;
        printf("Select Battleship (1-4): ");
        if (scanf("%d", &temp) == 1 && temp >= 1 && temp <= 4) {
            selectedBType = temp - 1;
            break;
        }
        clearLastLine(); clearInputBuffer();
    }
    while (true) {
        printf("Max speed of Battleship's shell in m/s (1 - 200): ");
        if (scanf("%d", &vMax) == 1 && vMax > 0 && vMax <= 200) break;
        clearLastLine(); clearInputBuffer();
    }
}

void configureSimulations() {
    printf("\n========== Configure Advanced Simulation ==========\n\n");
    while (true) {
        printf("Reload Time of B between shots (T_B) in sec: ");
        if (scanf("%lf", &reloadTimeB) == 1 && reloadTimeB >= 0.0) break;
        clearLastLine(); clearInputBuffer();
    }
    while (true) {
        printf("Number of movement iterations 'k': ");
        if (scanf("%d", &k_iterations) == 1 && k_iterations > 0 && k_iterations <= MAX_POINTS) break;
        clearLastLine(); clearInputBuffer();
    }
    while (true) {
        printf("Iteration 't' where gun jams (0 to %d): ", k_iterations - 1);
        if (scanf("%d", &t_jam) == 1 && t_jam >= 0 && t_jam < k_iterations) break;
        clearLastLine(); clearInputBuffer();
    }
    while (true) {
        printf("Minimum vertical angle after jam (0 - 30 deg): ");
        if (scanf("%lf", &theta_min_B) == 1 && theta_min_B > 0.0 && theta_min_B <= 30.0) break;
        clearLastLine(); clearInputBuffer();
    }
    printf("\n");
}

// ============= SIMULATION HELPER FUNCTIONS ===================

double calculateDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

double calculateRange(double velocity, double angleDeg) {
    return (velocity * velocity * sin(2.0 * DEG_TO_RAD(angleDeg))) / GRAV;
}

double calculateFlightTime(double velocity, double angleDeg) {
    return (2.0 * velocity * sin(DEG_TO_RAD(angleDeg))) / GRAV;
}

double getOptimalAngle(double minAngle, double maxAngle) {
    if (minAngle <= 45.0 && maxAngle >= 45.0) return 45.0;
    if (maxAngle < 45.0) return maxAngle;
    return minAngle;
}

double getWorstAngle(double minAngle, double maxAngle) {
    double distMin = fabs(minAngle - 45.0);
    double distMax = fabs(maxAngle - 45.0);
    return (distMin >= distMax) ? minAngle : maxAngle;
}

double getMaxRangeB() { return ((double)vMax * (double)vMax) / GRAV; }

double getMaxRangeE(int idx) {
    return calculateRange((double)E_ships[idx].typeInfo.maxBulletVelo, getOptimalAngle(E_ships[idx].typeInfo.minAngle, E_ships[idx].typeInfo.maxAngle));
}

double getMinRangeE(int idx) {
    if (E_ships[idx].typeInfo.minBulletVelo == 0) return 0.0;
    return calculateRange((double)E_ships[idx].typeInfo.minBulletVelo, getWorstAngle(E_ships[idx].typeInfo.minAngle, E_ships[idx].typeInfo.maxAngle));
}

int canEscortHitB(int idx, double bx, double by) {
    double dist = calculateDistance(bx, by, (double)E_ships[idx].x, (double)E_ships[idx].y);
    return dist >= getMinRangeE(idx) && dist <= getMaxRangeE(idx);
}

void findBFiringParams(double dist, double minAngleAllowed, double *outAngle, double *outVelocity, double *outFlightTime) {
    if (dist <= 0.0001) {
        *outAngle = (45.0 >= minAngleAllowed) ? 45.0 : minAngleAllowed;
        *outVelocity = 0.0; *outFlightTime = 0.0; return;
    }
    double v45 = sqrt(dist * GRAV);
    if (v45 <= (double)vMax) {
        *outAngle = 45.0; *outVelocity = v45;
    } else {
        *outVelocity = (double)vMax;
        double sinVal = (dist * GRAV) / ((double)vMax * (double)vMax);
        if (sinVal > 1.0) sinVal = 1.0;
        double lowAngle = RAD_TO_DEG(asin(sinVal)) / 2.0;
        *outAngle = (lowAngle >= minAngleAllowed) ? lowAngle : (90.0 - lowAngle); 
    }
    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

void findEFiringParams(int idx, double dist, double *outAngle, double *outVelocity, double *outFlightTime) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    if (dist <= 0.0001) {
        *outAngle = info->minAngle; *outVelocity = 0.0; *outFlightTime = 0.0; return;
    }
    double step = 0.1;
    for (double a = info->minAngle; a <= info->maxAngle; a += step) {
        double s2a = sin(2.0 * DEG_TO_RAD(a));
        if (s2a <= 0.0001) continue;
        double vReq = sqrt((dist * GRAV) / s2a);
        if (vReq >= (double)info->minBulletVelo && vReq <= (double)info->maxBulletVelo) {
            *outAngle = a; *outVelocity = vReq; *outFlightTime = calculateFlightTime(vReq, a); return;
        }
    }
    *outAngle = getOptimalAngle(info->minAngle, info->maxAngle);
    *outVelocity = (double)info->maxBulletVelo;
    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

// ============= STRATEGY & SORTING LOGIC ===================

int compareAttackTargets(const void *a, const void *b) {
    AttackTarget *t1 = (AttackTarget *)a;
    AttackTarget *t2 = (AttackTarget *)b;
    
    // 1. Sort by Threat Score (Descending)
    if (t1->threat_score > t2->threat_score) return -1;
    if (t1->threat_score < t2->threat_score) return 1;
    
    // 2. If threat is 0 for both (cannot hit B), shoot closest first
    if (t1->dist < t2->dist) return -1;
    if (t1->dist > t2->dist) return 1;
    return 0;
}

int compareEAttacks(const void *a, const void *b) {
    E_Attack *e1 = (E_Attack *)a;
    E_Attack *e2 = (E_Attack *)b;
    if (e1->t_hit_B < e2->t_hit_B) return -1;
    if (e1->t_hit_B > e2->t_hit_B) return 1;
    return 0;
}

// ============= BATTLE SIMULATION LOGIC ===================

SimResult simulateBattleStep(double bx, double by, double minAngleAllowedB, double *b_total_damage) {
    SimResult result;
    result.battleshipSunk = 0; result.sunkByEscortIndex = -1; result.escortHitCount = 0;
    
    double maxRangeB = getMaxRangeB();
    AttackTarget targets[MAX_ESCORTS];
    int t_count = 0;

    // 1. Gather all E ships in B's range and calculate Threat Score
    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;
        double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
        
        if (dist <= maxRangeB) {
            targets[t_count].e_index = i;
            targets[t_count].dist = dist;
            findBFiringParams(dist, minAngleAllowedB, &targets[t_count].b_angle, &targets[t_count].b_velocity, &targets[t_count].ft_B_to_E);
            
            if (canEscortHitB(i, bx, by)) {
                double ea, ev, eft;
                findEFiringParams(i, dist, &ea, &ev, &eft);
                targets[t_count].ft_E_to_B = eft;
                targets[t_count].impact = E_ships[i].typeInfo.impactpower;
                targets[t_count].threat_score = targets[t_count].impact / eft; // The Heuristic!
            } else {
                targets[t_count].ft_E_to_B = -1.0;
                targets[t_count].impact = 0.0;
                targets[t_count].threat_score = 0.0; 
            }
            t_count++;
        }
    }

    // 2. Apply Custom Strategy: Sort targets based on Threat Score
    qsort(targets, t_count, sizeof(AttackTarget), compareAttackTargets);
    
    result.attackOrderCount = t_count;
    for (int i = 0; i < t_count; i++) result.attackOrder[i] = E_ships[targets[i].e_index].index;

    // 3. Gather all E ships that can hit B
    E_Attack e_attacks[MAX_ESCORTS];
    int e_atk_count = 0;
    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;
        if (canEscortHitB(i, bx, by)) {
            double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
            double ea, ev, eft; findEFiringParams(i, dist, &ea, &ev, &eft);
            e_attacks[e_atk_count++] = (E_Attack){i, eft, E_ships[i].typeInfo.impactpower, 0};
        }
    }

    // 4. Simulate Timeline - B firing and preempting E's attacks
    double b_fire_time = 0.0;
    for (int i = 0; i < t_count; i++) {
        double impact_time_on_E = b_fire_time + targets[i].ft_B_to_E;
        
        // Did B's shot destroy E before E's bullet hit B?
        for (int j = 0; j < e_atk_count; j++) {
            if (e_attacks[j].e_idx == targets[i].e_index) {
                if (impact_time_on_E <= e_attacks[j].t_hit_B) {
                    e_attacks[j].canceled = 1; // Preempted! No damage to B.
                }
            }
        }
        b_fire_time += reloadTimeB;
    }

    // 5. Apply valid E attacks cumulatively (Chronological Order)
    qsort(e_attacks, e_atk_count, sizeof(E_Attack), compareEAttacks);
    double b_sink_time = -1.0;
    
    for (int i = 0; i < e_atk_count; i++) {
        if (!e_attacks[i].canceled) {
            *b_total_damage += e_attacks[i].impact;
            if (*b_total_damage >= 1.0) {
                result.battleshipSunk = 1;
                result.sunkByEscortIndex = E_ships[e_attacks[i].e_idx].index;
                b_sink_time = e_attacks[i].t_hit_B;
                break; 
            }
        }
    }

    // 6. Record B's successful shots (Cancel shots if B was sunk before pulling the trigger)
    b_fire_time = 0.0;
    double max_sim_time = (result.battleshipSunk) ? b_sink_time : 0.0;

    for (int i = 0; i < t_count; i++) {
        if (result.battleshipSunk && b_fire_time >= b_sink_time) break; // B is dead, stops firing
        
        int e_idx = targets[i].e_index;
        E_ships[e_idx].alive = 0; 
        
        result.hits[result.escortHitCount] = (HitRecord){ E_ships[e_idx].index, targets[i].dist, targets[i].ft_B_to_E, targets[i].b_angle, targets[i].b_velocity, b_fire_time };
        result.escortHitCount++;
        
        double end_t = b_fire_time + targets[i].ft_B_to_E;
        if (end_t > max_sim_time) max_sim_time = end_t;
        
        b_fire_time += reloadTimeB;
    }
    
    result.battleDuration = max_sim_time;
    return result;
}

// ============= SIMULATION I/O FUNCTIONS ===================

void appendStepResultToFile(FILE *file, int step, int bx, int by, double minAngle, SimResult result, double b_total_damage) {
    fprintf(file, "---------- Iteration %d ----------\n", step + 1);
    fprintf(file, "B Position : (%d, %d)\n", bx, by);
    fprintf(file, "B Gun Min Angle : %.1f degrees\n\n", minAngle);
    
    // Print B's strategic attack order
    fprintf(file, "B's Strategic Attack Order : [ ");
    for(int i = 0; i < result.attackOrderCount; i++) fprintf(file, "E#%d ", result.attackOrder[i]);
    fprintf(file, "]\n\n");
    
    if (result.battleshipSunk) {
        fprintf(file, "OUTCOME : Battleship SUNK by Escort #%d!\n", result.sunkByEscortIndex);
        fprintf(file, "Total Damage taken before sinking: %.2f%%\n", b_total_damage * 100.0);
    } else {
        fprintf(file, "OUTCOME : Battleship Survived this step.\n");
        fprintf(file, "Current Total Damage: %.2f%%\n", b_total_damage * 100.0);
    }

    fprintf(file, "\nEscorts Destroyed by B: %d\n", result.escortHitCount);
    for (int i = 0; i < result.escortHitCount; i++) {
        HitRecord h = result.hits[i];
        fprintf(file, "  -> Hit Escort #%d | Fired at: %.1fs | Impact at: %.1fs\n", 
                h.escortIndex, h.fireTime, h.fireTime + h.flightTime);
    }
    fprintf(file, "\nStep Battle Duration: %.4f s\n\n", result.battleDuration);
}

void runSimulation(int simMode) {
    char filename[50];
    sprintf(filename, "Advanced_Sim_%d_Results.txt", simMode);
    
    FILE *file = fopen(filename, "w");
    if (!file) return;

    fprintf(file, "========== SIMULATION %d RESULTS (Part 2-A) ==========\n\n", simMode);

    resetBattlefield();
    printf("\n--- Running Simulation %d ---\n", simMode);

    double b_total_damage = 0.0; 

    for (int step = 0; step < k_iterations; step++) {
        int bx = b_path[step].x;
        int by = b_path[step].y;
        double currentMinAngle = (simMode == 2 && step >= t_jam) ? theta_min_B : 0.0;

        SimResult result = simulateBattleStep(bx, by, currentMinAngle, &b_total_damage);
        appendStepResultToFile(file, step, bx, by, currentMinAngle, result, b_total_damage);
        
        printf("Iteration %d: %s | Damage Taken: %.1f%% | E Destroyed: %d\n", 
               step + 1, result.battleshipSunk ? "SUNK!" : "Survived", b_total_damage * 100.0, result.escortHitCount);

        if (result.battleshipSunk) break;
    }

    if (b_total_damage < 1.0) {
        fprintf(file, "\n========== FINAL OUTCOME ==========\n");
        fprintf(file, "Battleship SURVIVED all iterations!\n");
        fprintf(file, "Total Cumulative Impact Taken: %.2f%%\n", b_total_damage * 100.0);
    }

    fclose(file);
    printf("Results saved to '%s'\n", filename);
}

// ============= MAIN FUNCTION ========================

int main() {
    srand((unsigned int)time(NULL));
    printHeader();
    configureArena();
    configureBattleship();
    configureSimulations();
    
    addETypeValues(vMax);
    generateEInstances();
    generateBPath();

    runSimulation(1); // Normal Path
    runSimulation(2); // Jammed Gun

    printf("\nSimulations Complete!\nPress Enter to exit...");
    clearInputBuffer(); getchar();
    return 0;
}
