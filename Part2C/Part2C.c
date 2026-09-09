#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// =============== CONSTANTS ===============
#define MAX_ESCORTS 100
#define MAX_POINTS 100 
#define MAX_BULLETS 50000 
#define MAX_LOGGED_HITS 2000 // Maximum hits to log per iteration to prevent huge files
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
    double reloadTime; 
    double gamma; 
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
    int active;
    int is_b_to_e; 
    int target_idx; 
    double impact_time;
    double impact_power;
} Bullet;

// Hit Log Structures for Text File Output
typedef struct {
    int e_ship_index;
    double impact_time;
    double damage_caused;
    int is_fatal; 
} Log_B_Hit;

typedef struct {
    int e_ship_index;
    double impact_time;
    double damage_caused;
} Log_E_Hit;

typedef struct {
    int battleshipSunk;          
    int sunkByEscortIndex;       
    double battleDuration;
    
    int attackOrder[MAX_ESCORTS]; 
    int attackOrderCount;
    
    int b_hits_landed;
    int e_hits_landed;
    int escortHitCount; 
    
    // Arrays to store individual bullet logs
    Log_B_Hit b_log[MAX_LOGGED_HITS];
    int b_log_count;
    
    Log_E_Hit e_log[MAX_LOGGED_HITS];
    int e_log_count;
} SimResult;

// ============= GLOBAL VARIABLES ============
int canvasSize = 5000;
int vMax;
int noEscort = 1;
Escortship E_ships[MAX_ESCORTS];
Escortship original_E_ships[MAX_ESCORTS]; 

int selectedBType = 0;    
int k_iterations = 5; 
int t_jam = 2;        
double theta_min_B = 20.0;
double reloadTimeB = 2.0; 
double gamma_B = 0.005; 

Point b_path[MAX_POINTS];

double e_damage[MAX_ESCORTS] = {0.0};
int n_B_fires = 0;
int n_E_fires[MAX_ESCORTS] = {0};

TypeInfo_E info_ETypes[5] = {
    {"EA", "1936A-class Destroyer",   "SK C/34 naval gun",        0.08, 20.0, 0.0, 0.0, 0, 0, 0.0, 0.0},
    {"EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun",    0.06, 30.0, 0.0, 0.0, 0, 0, 0.0, 0.0},
    {"EC", "Matsu-class Destroyer",   "Type 89 dual-purpose gun", 0.07, 25.0, 0.0, 0.0, 0, 0, 0.0, 0.0},
    {"ED", "F-class Escort Ships",    "SK c/32 naval gun",        0.05, 50.0, 0.0, 0.0, 0, 0, 0.0, 0.0},
    {"EE", "Japanese Kaibokan",       "(4.7 inch) naval guns",    0.04, 70.0, 0.0, 0.0, 0, 0, 0.0, 0.0}
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
void clearInputBuffer() { int c; while ((c = getchar()) != '\n' && c != EOF); }
void clearLastLine() { printf("\033[1A\033[2K\r"); }
int genRanIntBetween(int min, int max) { return min + rand() % (max - min + 1); }
double genRanDoubleBetween(double min, double max) {
    double scale = (double)rand() / (double)RAND_MAX;
    return min + scale * (max - min);
}

void addETypeValues(int vMax) {
    for (int i = 0; i < num_ETypes; i++) {
        double maxPossAngle = 90.00 - info_ETypes[i].angleRange;
        info_ETypes[i].minAngle = genRanDoubleBetween(0, maxPossAngle);
        info_ETypes[i].maxAngle = info_ETypes[i].minAngle + info_ETypes[i].angleRange;
        info_ETypes[i].maxBulletVelo = genRanIntBetween(1, vMax);
        info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo - 1);

        if (strcmp(info_ETypes[i].typeNotation, "EA") == 0) {
            info_ETypes[i].maxBulletVelo = (int)(1.2 * vMax);
            info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo);
        }
        
        info_ETypes[i].reloadTime = genRanDoubleBetween(2.0, 6.0); 
        info_ETypes[i].gamma = genRanDoubleBetween(0.01, 0.05); 
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
        e_damage[i] = 0.0;
        n_E_fires[i] = 0;
    }
    n_B_fires = 0;
}

// ============= UI & INPUT FUNCTIONS ===================
void printHeader() {
    printf("======================================\n");
    printf("|        BATTLESHIP SIMULATOR        |\n");
    printf("|     Part 2 - C (Full Physics)      |\n");
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
    for (int i = 0; i < num_BTypes; i++) printf("%d. %c - %s\n", (i + 1), info_BTypes[i].typeNotation, info_BTypes[i].name);
    printf("\n");
    while (true) {
        int temp;
        printf("Select Battleship (1-4): ");
        if (scanf("%d", &temp) == 1 && temp >= 1 && temp <= 4) {
            selectedBType = temp - 1; break;
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
    gamma_B = genRanDoubleBetween(0.001, 0.009);
    printf("\n> B Ship Gamma (degradation rate) generated: %.4f\n\n", gamma_B);
}

// ============= SIMULATION HELPER FUNCTIONS ===================
double calculateDistance(double x1, double y1, double x2, double y2) { return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); }
double calculateRange(double velocity, double angleDeg) { return (velocity * velocity * sin(2.0 * DEG_TO_RAD(angleDeg))) / GRAV; }
double calculateFlightTime(double velocity, double angleDeg) { return (2.0 * velocity * sin(DEG_TO_RAD(angleDeg))) / GRAV; }
double getOptimalAngle(double minAngle, double maxAngle) {
    if (minAngle <= 45.0 && maxAngle >= 45.0) return 45.0;
    if (maxAngle < 45.0) return maxAngle;
    return minAngle;
}
double getWorstAngle(double minAngle, double maxAngle) {
    double distMin = fabs(minAngle - 45.0), distMax = fabs(maxAngle - 45.0);
    return (distMin >= distMax) ? minAngle : maxAngle;
}
double getMaxRangeB() { return ((double)vMax * (double)vMax) / GRAV; }
double getMaxRangeE(int idx) { return calculateRange((double)E_ships[idx].typeInfo.maxBulletVelo, getOptimalAngle(E_ships[idx].typeInfo.minAngle, E_ships[idx].typeInfo.maxAngle)); }
double getMinRangeE(int idx) {
    if (E_ships[idx].typeInfo.minBulletVelo == 0) return 0.0;
    return calculateRange((double)E_ships[idx].typeInfo.minBulletVelo, getWorstAngle(E_ships[idx].typeInfo.minAngle, E_ships[idx].typeInfo.maxAngle));
}
int canEscortHitB(int idx, double bx, double by) {
    double dist = calculateDistance(bx, by, (double)E_ships[idx].x, (double)E_ships[idx].y);
    return dist >= getMinRangeE(idx) && dist <= getMaxRangeE(idx);
}

void findBFiringParams(double dist, double minAngleAllowed, double *outAngle, double *outVelocity, double *outFlightTime) {
    if (dist <= 0.0001) { *outAngle = (45.0 >= minAngleAllowed) ? 45.0 : minAngleAllowed; *outVelocity = 0.0; *outFlightTime = 0.0; return; }
    double v45 = sqrt(dist * GRAV);
    if (v45 <= (double)vMax) { *outAngle = 45.0; *outVelocity = v45; } 
    else {
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
    if (dist <= 0.0001) { *outAngle = info->minAngle; *outVelocity = 0.0; *outFlightTime = 0.0; return; }
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

// ============= STRATEGY & EVENT ENGINE ===================

int selectBTarget(double bx, double by, Bullet bullets[], int max_b) {
    double maxRangeB = getMaxRangeB();
    int best_target = -1;
    double highest_threat = -1.0;

    double pending_dmg[MAX_ESCORTS] = {0.0};
    for (int i = 0; i < max_b; i++) {
        if (bullets[i].active && bullets[i].is_b_to_e) {
            pending_dmg[bullets[i].target_idx] += bullets[i].impact_power;
        }
    }

    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;
        if (e_damage[i] + pending_dmg[i] >= 1.0) continue;

        double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
        if (dist <= maxRangeB) {
            double threat = 0.0;
            if (canEscortHitB(i, bx, by)) {
                double ea, ev, eft;
                findEFiringParams(i, dist, &ea, &ev, &eft);
                double current_ip_E = E_ships[i].typeInfo.impactpower * exp(-E_ships[i].typeInfo.gamma * n_E_fires[i]);
                double dps = current_ip_E / E_ships[i].typeInfo.reloadTime;
                threat = dps / eft; 
            } else {
                threat = 0.0001 / dist; 
            }

            if (threat > highest_threat) {
                highest_threat = threat;
                best_target = i;
            }
        }
    }
    return best_target;
}

SimResult simulateBattleStepEventDriven(double bx, double by, double minAngleAllowedB, double *b_total_damage) {
    SimResult result;
    result.battleshipSunk = 0; result.sunkByEscortIndex = -1;
    result.attackOrderCount = 0; result.b_hits_landed = 0; result.e_hits_landed = 0;
    result.escortHitCount = 0; result.b_log_count = 0; result.e_log_count = 0;

    double current_time = 0.0;
    double next_b_fire = 0.0;
    double next_e_fire[MAX_ESCORTS];
    
    for (int i = 0; i < noEscort; i++) {
        if (E_ships[i].alive && canEscortHitB(i, bx, by)) next_e_fire[i] = 0.0;
        else next_e_fire[i] = 999999.0;
    }

    Bullet bullets[MAX_BULLETS];
    int num_bullets = 0;

    while (!result.battleshipSunk && current_time < 5000.0) { 
        double min_t = 999999.0;
        int event_type = -1; 
        int event_idx = -1;

        if (next_b_fire < min_t) { min_t = next_b_fire; event_type = 0; }
        for (int i = 0; i < noEscort; i++) {
            if (next_e_fire[i] < min_t) { min_t = next_e_fire[i]; event_type = 1; event_idx = i; }
        }
        for (int i = 0; i < num_bullets; i++) {
            if (bullets[i].active && bullets[i].impact_time < min_t) { min_t = bullets[i].impact_time; event_type = 2; event_idx = i; }
        }

        if (min_t >= 999999.0) break; 
        current_time = min_t;

        if (event_type == 0) { 
            // B FIRES
            int target_e = selectBTarget(bx, by, bullets, num_bullets);
            if (target_e != -1) {
                double ba, bv, bft;
                double dist = calculateDistance(bx, by, (double)E_ships[target_e].x, (double)E_ships[target_e].y);
                findBFiringParams(dist, minAngleAllowedB, &ba, &bv, &bft);
                
                double current_ip_B = 1.0 * exp(-gamma_B * n_B_fires); 
                n_B_fires++;

                if (num_bullets < MAX_BULLETS) {
                    bullets[num_bullets++] = (Bullet){1, 1, target_e, current_time + bft, current_ip_B};
                }
                
                if (result.attackOrderCount < MAX_ESCORTS) {
                    // Prevent duplicates in sequential sequence tracking
                    if (result.attackOrderCount == 0 || result.attackOrder[result.attackOrderCount-1] != E_ships[target_e].index) {
                        result.attackOrder[result.attackOrderCount++] = E_ships[target_e].index;
                    }
                }
                next_b_fire = current_time + reloadTimeB;
            } else {
                next_b_fire = 999999.0; 
            }
        } 
        else if (event_type == 1) { 
            // E FIRES
            double ba, bv, bft;
            double dist = calculateDistance(bx, by, (double)E_ships[event_idx].x, (double)E_ships[event_idx].y);
            findEFiringParams(event_idx, dist, &ba, &bv, &bft);
            
            double ip0 = E_ships[event_idx].typeInfo.impactpower;
            double current_ip_E = ip0 * exp(-E_ships[event_idx].typeInfo.gamma * n_E_fires[event_idx]);
            n_E_fires[event_idx]++;

            if (num_bullets < MAX_BULLETS) {
                bullets[num_bullets++] = (Bullet){1, 0, event_idx, current_time + bft, current_ip_E};
            }
            next_e_fire[event_idx] = current_time + E_ships[event_idx].typeInfo.reloadTime;
        } 
        else if (event_type == 2) { 
            // BULLET IMPACTS
            Bullet *b = &bullets[event_idx];
            b->active = 0;
            
            if (b->is_b_to_e) {
                // B's bullet hits E
                if (E_ships[b->target_idx].alive) {
                    e_damage[b->target_idx] += b->impact_power;
                    result.b_hits_landed++;
                    
                    int fatal = 0;
                    if (e_damage[b->target_idx] >= 1.0) {
                        E_ships[b->target_idx].alive = 0;
                        next_e_fire[b->target_idx] = 999999.0; 
                        fatal = 1;
                    }

                    // Log this hit!
                    if (result.b_log_count < MAX_LOGGED_HITS) {
                        result.b_log[result.b_log_count].e_ship_index = E_ships[b->target_idx].index;
                        result.b_log[result.b_log_count].impact_time = b->impact_time;
                        result.b_log[result.b_log_count].damage_caused = b->impact_power;
                        result.b_log[result.b_log_count].is_fatal = fatal;
                        result.b_log_count++;
                    }
                }
            } else {
                // E's bullet hits B
                *b_total_damage += b->impact_power;
                result.e_hits_landed++;
                
                // Log this hit!
                if (result.e_log_count < MAX_LOGGED_HITS) {
                    result.e_log[result.e_log_count].e_ship_index = E_ships[b->target_idx].index;
                    result.e_log[result.e_log_count].impact_time = b->impact_time;
                    result.e_log[result.e_log_count].damage_caused = b->impact_power;
                    result.e_log_count++;
                }

                if (*b_total_damage >= 1.0) {
                    result.battleshipSunk = 1;
                    result.sunkByEscortIndex = E_ships[b->target_idx].index;
                }
            }
        }
    }

    result.battleDuration = current_time;
    for (int i=0; i<noEscort; i++) {
        if (!E_ships[i].alive && original_E_ships[i].alive) result.escortHitCount++;
    }

    return result;
}

// ============= SIMULATION I/O FUNCTIONS ===================

void appendStepResultToFile(FILE *file, int step, int bx, int by, double minAngle, SimResult result, double b_total_damage) {
    fprintf(file, "---------- Iteration %d ----------\n", step + 1);
    fprintf(file, "B Position : (%d, %d)\n", bx, by);
    fprintf(file, "B Gun Min Angle : %.1f degrees\n\n", minAngle);
    
    fprintf(file, "B's Attack Target Sequence (Ordered) : [ ");
    for(int i = 0; i < result.attackOrderCount; i++) fprintf(file, "E#%d ", result.attackOrder[i]);
    fprintf(file, "]\n\n");
    
    if (result.battleshipSunk) {
        fprintf(file, "OUTCOME : Battleship SUNK by Escort #%d!\n", result.sunkByEscortIndex);
        fprintf(file, "Total Damage taken before sinking: %.2f%%\n\n", b_total_damage * 100.0);
    } else {
        fprintf(file, "OUTCOME : Battleship Survived this step.\n");
        fprintf(file, "Current Total Damage: %.2f%%\n\n", b_total_damage * 100.0);
    }
    
    // ==========================================
    // DETAILED HIT LOGS (Added per your request)
    // ==========================================
    fprintf(file, "--- [ BATTESHIP'S HITS ON ESCORTS ] ---\n");
    if (result.b_log_count == 0) fprintf(file, "  None.\n");
    for (int i = 0; i < result.b_log_count; i++) {
        fprintf(file, "  Time: %6.2fs | Hit E#%d | Damage Done: %5.2f%% %s\n", 
                result.b_log[i].impact_time, result.b_log[i].e_ship_index, 
                result.b_log[i].damage_caused * 100.0, 
                result.b_log[i].is_fatal ? "[FATAL! E Ship Destroyed]" : "");
    }
    if (result.b_log_count == MAX_LOGGED_HITS) fprintf(file, "  ... (Hit log truncated)\n");
    
    fprintf(file, "\n--- [ ESCORTS' HITS ON BATTLESHIP ] ---\n");
    if (result.e_log_count == 0) fprintf(file, "  None.\n");
    for (int i = 0; i < result.e_log_count; i++) {
        fprintf(file, "  Time: %6.2fs | Fired By E#%d | Damage Taken: %5.2f%%\n", 
                result.e_log[i].impact_time, result.e_log[i].e_ship_index, 
                result.e_log[i].damage_caused * 100.0);
    }
    if (result.e_log_count == MAX_LOGGED_HITS) fprintf(file, "  ... (Hit log truncated)\n");
    
    fprintf(file, "\nStatistics:\n");
    fprintf(file, "  -> Total Escorts Destroyed so far: %d / %d\n", result.escortHitCount, noEscort);
    fprintf(file, "  -> B Ship Bullets Landed on Target: %d\n", result.b_hits_landed);
    fprintf(file, "  -> E Ship Bullets Landed on B: %d\n", result.e_hits_landed);
    fprintf(file, "  -> Iteration Battle Duration: %.4f s\n\n", result.battleDuration);
}

void runSimulation(int simMode) {
    char filename[50];
    sprintf(filename, "Final_Sim_%d_Results.txt", simMode);
    
    FILE *file = fopen(filename, "w");
    if (!file) return;

    fprintf(file, "========== SIMULATION %d RESULTS (Part 2-C) ==========\n", simMode);
    fprintf(file, "B Ship Gamma (Degradation) : %.5f\n\n", gamma_B);

    resetBattlefield();
    printf("\n--- Running Simulation %d ---\n", simMode);

    double b_total_damage = 0.0; 

    for (int step = 0; step < k_iterations; step++) {
        int bx = b_path[step].x;
        int by = b_path[step].y;
        double currentMinAngle = (simMode == 2 && step >= t_jam) ? theta_min_B : 0.0;

        SimResult result = simulateBattleStepEventDriven(bx, by, currentMinAngle, &b_total_damage);
        appendStepResultToFile(file, step, bx, by, currentMinAngle, result, b_total_damage);
        
        printf("Iteration %d: %s | Dmg: %.1f%% | B Hit %d times | E Destroyed: %d\n", 
               step + 1, result.battleshipSunk ? "SUNK!" : "Survived", b_total_damage * 100.0, result.e_hits_landed, result.escortHitCount);

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
