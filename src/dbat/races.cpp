#include "dbat/races.h"

#include <utility>
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/weather.h"

const char *race_abbrevs[NUM_RACES + 1] = {
        "Hum",
        "Sai",
        "Ice",
        "Kon",
        "Nam",
        "Mut",
        "Kan",
        "H-B",
        "Bio",
        "And",
        "Dem",
        "Maj",
        "Kai",
        "Tru",
        "Hos",
        "Ict",
        "Sab",
        "Ser",
        "Trl",
        "Dra",
        "Arl",
        "Mnd",
        "Mec",
        "Spi",
        "\n"
};

#define Y   true
#define N   false


const char *race_display[NUM_RACES] = {
        "@B1@W) @cHuman\r\n",
        "@B2@W) @cSaiyan\r\n",
        "@B3@W) @cIcer\r\n",
        "@B4@W) @cKonatsu\r\n",
        "@B5@W) @cNamekian\r\n",
        "@B6@W) @cMutant\r\n",
        "@B7@W) @cKanassan\r\n",
        "@B8@W) @cHalf Breed\r\n",
        "@B9@W) @cBio-Android\r\n",
        "@B10@W) @cAndroid\r\n",
        "@B11@W) @cDemon\r\n",
        "@B12@W) @cMajin\r\n",
        "@B13@W) @cKai\r\n",
        "@B14@W) @cTruffle\r\n",
        "@B15@W) @cHoshijin\r\n",
        "@B16@W) @YArlian\r\n",
        "@B17@W) @GAnimal\r\n",
        "@B18@W) @MSaiba\r\n",
        "@B19@W) @BSerpent\r\n",
        "@B20@W) @ROgre\r\n",
        "@B21@W) @CYardratian\r\n",
        "@B22@W) @GLizardfolk\r\n",
        "@B23@W) @GMechanical\r\n",
        "@B24@W) @MSpirit\r\n",
};

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
int racial_ability_mods[][6] = {
/*                      Str,Con,Int,Wis,Dex,Cha */
/* RACE_HUMAN       */ {0,  0,  0,  0,  0,  0},
/* RACE_SAIYAN      */
                       {0,  -2, 0,  0,  2,  0},
/* RACE_ICER        */
                       {-2, 2,  0,  0,  0,  0},
/* RACE_KONATSU     */
                       {0,  2,  0,  0,  0,  -2},
/* RACE_NAMEK       */
                       {0,  0,  0,  0,  0,  0},
/* RACE_MUTANT      */
                       {-2, 0,  0,  0,  2,  0},
/* RACE_KANASSAN    */
                       {0,  -2, 2,  0,  2,  2},
/* RACE_HALFBREED   */
                       {2,  0,  -2, 0,  0,  -2},
/* RACE_BIO         */
                       {0,  0,  0,  0,  0,  0},
/* RACE_ANDROID     */
                       {0,  0,  0,  0,  0,  0},
/* RACE_DEMON       */
                       {0,  0,  0,  0,  0,  0},
/* RACE_MAJIN       */
                       {0,  0,  0,  0,  0,  0},
/* RACE_KAI         */
                       {0,  0,  0,  0,  0,  0},
/* RACE_TRUFFLE     */
                       {14, 8,  -4, 0,  -2, -4},
/* RACE_GOBLIN      */
                       {-2, 0,  0,  0,  2,  -2},
/* RACE_ANIMAL      */
                       {0,  0,  0,  0,  0,  0},
/* RACE_ORC         */
                       {4,  0,  -2, -2, 0,  -2},
/* RACE_SNAKE       */
                       {0,  0,  0,  0,  0,  0},
/* RACE_TROLL       */
                       {12, 12, -4, -2, 4,  -4},
/* RACE_MINOTAUR    */
                       {8,  4,  -4, 0,  0,  -2},
/* RACE_KOBOLD      */
                       {-4, -2, 0,  0,  2,  0},
/* RACE_LIZARDFOLK  */
                       {0,  0,  0,  0,  0,  0},
/* RACE_WARHOST     */
                       {0,  0,  0,  0,  0,  0},
/* RACE_FAERIE      */
                       {0,  0,  0,  0,  0,  0},
                       {0,  0,  0,  0,  0}
};

void racial_ability_modifiers(struct char_data *ch) {
    int chrace = 0;
    if (GET_RACE(ch) >= NUM_RACES || GET_RACE(ch) < 0) {
        basic_mud_log("SYSERR: Unknown race %d in racial_ability_modifiers", GET_RACE(ch));
    } else {
        chrace = GET_RACE(ch);
    }

    /*ch->real_abils.str += racial_ability_mods[chrace][0];
    ch->real_abils.con += racial_ability_mods[chrace][1];
    ch->real_abils.intel += racial_ability_mods[chrace][2];
    ch->real_abils.wis += racial_ability_mods[chrace][3];
    ch->real_abils.dex += racial_ability_mods[chrace][4];*/
}


/* Converted into metric units: cm and kg; SRD has english units. */
struct {
    int height[NUM_SEX];    /* cm */
    int heightdie;    /* 2d(heightdie) added to height */
    int weight[NUM_SEX];    /* kg */
    int weightfac;    /* added height * weightfac/100 added to weight */
} hw_info[NUM_RACES] = {
/* RACE_HUMAN      */ {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SAIYAN     */
                      {{140, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ICER       */
                      {{100, 111, 95},  10, {17, 18, 16}, 18},
/* RACE_KONATSU    */
                      {{121, 124, 109}, 20, {52, 59, 45}, 125},
/* RACE_NAMEK      */
                      {{137, 140, 135}, 20, {40, 45, 36}, 89},
/* RACE_MUTANT     */
                      {{141, 150, 140}, 10, {46, 54, 39}, 89},
/* RACE_KANASSAN   */
                      {{135, 135, 135}, 15, {37, 39, 36}, 63},
/* RACE_HALFBREED  */
                      {{141, 147, 135}, 30, {59, 68, 50}, 125},
/* RACE_BIO        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANDROID    */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_DEMON      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MAJIN      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KAI        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TRUFFLE    */
                      {{40,  50,  45},  16, {16, 24, 9},  8},
/* RACE_GOBLIN     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANIMAL     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ORC        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SNAKE      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TROLL      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MINOTAUR   */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KOBOLD     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_LIZARDFOLK */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_WARHOST    */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_FAERIE     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89}
};


void set_height_and_weight_by_race(struct char_data *ch) {
    int race, sex, mod;

    if (!IS_NPC(ch)) {
        return;
    }

    race = GET_RACE(ch);
    sex = GET_SEX(ch);
    if (sex < SEX_NEUTRAL || sex >= NUM_SEX) {
        basic_mud_log("Invalid gender in set_height_and_weight_by_race: %d", sex);
        sex = SEX_NEUTRAL;
    }
    if (race <= RACE_UNDEFINED || race >= NUM_RACES) {
        basic_mud_log("Invalid gender in set_height_and_weight_by_race: %d", GET_SEX(ch));
        race = RACE_UNDEFINED + 1; /* first defined race */
    }

    mod = dice(2, hw_info[race].heightdie);
    ch->setHeight( hw_info[race].height[sex] + mod);
    mod *= hw_info[race].weightfac;
    mod /= 100;
    GET_WEIGHT(ch) = hw_info[race].weight[sex] + mod;
}


int invalid_race(struct char_data *ch, struct obj_data *obj) {
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
        return false;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_SAIYAN) && IS_SAIYAN(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_ICER) && IS_ICER(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_KONATSU) && IS_KONATSU(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ONLY_HUMAN) && !IS_HUMAN(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ONLY_ICER) && !IS_ICER(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ONLY_SAIYAN) && !IS_SAIYAN(ch))
        return true;

    if (OBJ_FLAGGED(obj, ITEM_ONLY_KONATSU) && !IS_KONATSU(ch))
        return true;

    return false;
}


int get_size(struct char_data *ch) {
    return ch->size != SIZE_UNDEFINED ? ch->size : ch->race->getSize();
}


const int size_bonus_table[NUM_SIZES] = {
/* XTINY */    8,
/* TINY */    4,
/* XSMALL */    2,
/* SMALL */    1,
/* MEDIUM */    0,
/* LARGE */    -1,
/* HUGE */    -2,
/* GIGANTIC */    -4,
/* COLOSSAL */    -8
};


int get_size_bonus(int sz) {
    if (sz < 0 || sz >= NUM_SIZES)
        sz = SIZE_MEDIUM;
    return size_bonus_table[sz];
}


int wield_type(int chsize, const struct obj_data *weap) {
    if (GET_OBJ_TYPE(weap) != ITEM_WEAPON) {
        return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
    } else if (chsize > GET_OBJ_SIZE(weap)) {
        return WIELD_LIGHT;
    } else if (chsize == GET_OBJ_SIZE(weap)) {
        return WIELD_ONEHAND;
    } else if (chsize == GET_OBJ_SIZE(weap) - 1) {
        return WIELD_TWOHAND;
    } else if (chsize < GET_OBJ_SIZE(weap) - 1) {
        return WIELD_NONE; /* It's just too big for you! */
    } else {
        basic_mud_log("unknown size vector in wield_type: chsize=%d, weapsize=%d", chsize, GET_OBJ_SIZE(weap));
        return WIELD_NONE;
    }
}

int race_bodyparts[NUM_RACES][NUM_WEARS] = {
        /* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22 */
        /* U, F, F, N, N, B, H, L, F, H, A, U, A, W, W, W, W, W, B, E, E, W, M */
/* RACE_HUMAN       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_SAIYAN      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ICER        */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KONATSU     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_NAMEK       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_MUTANT      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KANASSAN    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_HALFBREED   */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_BIO         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ANDROID     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_DEMON       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_MAJIN       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KAI         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_TRUFFLE     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_HOSHIJIN    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ANIMAL      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_ORC         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_SNAKE       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_TROLL       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_MINOTAUR    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_ARLIAN      */
                       {0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_LIZARDFOLK  */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_WARHOST     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_FAERIE      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

void racial_body_parts(struct char_data *ch) {

    auto parts = race_bodyparts[GET_RACE(ch)];

    for (auto i = 1; i < NUM_WEARS; i++) {
        ch->bodyparts.set(i, parts[i]);
    }
}

namespace race {

    transform_bonus base_form = {0, 1, 0, 0};
    transform_bonus oozaru = {.bonus = 10000, .mult=2, .drain=0, .flag=PLR_OOZARU};

    Race::Race(race_id rid, const std::string &name, std::string abbr, int size, bool pc) {
        this->r_id = rid;
        this->name = name;
        this->lower_name = name;
        race_abbr = std::move(abbr);
        disg = "A Disguised " + name;
        // just generate lower name from the name.
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        pc_check = pc;
        race_size = size;
    }

    race_id Race::getID() const {
        return r_id;
    }

    const std::string &Race::getName() const {
        return name;
    }

    const std::string &Race::getNameLower() const {
        return lower_name;
    }

    const std::string &Race::getAbbr() const {
        return race_abbr;
    }

    const std::string &Race::getDisguised() const {
        return disg;
    }

    int Race::getSize() const {
        return race_size;
    }

    bool Race::isPcOk() const {
        return pc_check;
    }

    bool Race::isValidSex(int sex_id) const {
        switch (r_id) {
            case namekian:
                return sex_id == SEX_NEUTRAL;
            default:
                return true;
        }
    }

    bool Race::raceCanBeMimiced() const {
        switch (r_id) {
            case icer:
            case namekian:
            case bio:
            case demon:
            case majin:
            case hoshijin:
            case arlian:
                return false;
            default:
                return isPcOk();
        }
    }

    int Race::getRPPCost() const {
        switch (r_id) {
            case saiyan:
                return 60;
            case bio:
                return 35;
            case majin:
                return 55;
            case hoshijin:
                return 14;
            default:
                return 0;
        }
    }

    bool Race::raceCanTransform() const {
        switch (r_id) {
            case human:
            case kai:
            case truffle:
            case konatsu:
            case mutant:
            case icer:
            case halfbreed:
            case namekian:
            case saiyan:
            case bio:
            case majin:
            case android:
                return true;
            default:
                return false;

        }
    }

    bool Race::raceCanRevert() const {
        if (!raceCanTransform()) {
            return false;
        }
        switch (r_id) {
            case majin:
            case android:
            case bio:
            case truffle:
                return false;
            default:
                return true;
        }
    }

    bool Race::checkCanTransform(char_data *ch) const {
        // No point checking for Saiyan/Halfbreed because it's just as expensive to check for
        // the Oozaru flag.
        if (PLR_FLAGGED(ch, PLR_OOZARU)) {
            send_to_char(ch, "You are the great Oozaru right now and can't transform!\r\n");
            return false;
        }

        if (GET_KAIOKEN(ch) > 0) {
            send_to_char(ch, "You are in kaioken right now and can't transform!\r\n");
            return false;
        }

        return true;
    }


    int Race::getMaxTransformTier(char_data *ch) const {
        switch (r_id) {
            case saiyan:
                if (PLR_FLAGGED(ch, PLR_LSSJ)) {
                    return 2;
                } else {
                    return 4;
                }
            case human:
            case icer:
            case namekian:
            case bio:
                return 4;
            case truffle:
            case mutant:
            case halfbreed:
            case majin:
            case kai:
            case konatsu:
                return 3;
            case android:
                return 6;
            default:
                return 0;
        }
    }

    static const std::unordered_map<std::string, int> no_form_map{};
    static const std::unordered_map<std::string, int> two_form_map = {{"first",  1},
                                                                      {"second", 2}};
    static const std::unordered_map<std::string, int> three_form_map = {{"first",  1},
                                                                        {"second", 2},
                                                                        {"third",  3}};
    static const std::unordered_map<std::string, int> four_form_map = {{"first",  1},
                                                                       {"second", 2},
                                                                       {"third",  3},
                                                                       {"fourth", 4}};
    static const std::unordered_map<std::string, int> bio_form_map = {{"mature",        1},
                                                                      {"semi-perfect",  2},
                                                                      {"perfect",       3},
                                                                      {"super-perfect", 4}};
    static const std::unordered_map<std::string, int> android_form_map = {{"1.0", 1},
                                                                          {"2.0", 2},
                                                                          {"3.0", 3},
                                                                          {"4.0", 4},
                                                                          {"5.0", 5},
                                                                          {"6.0", 6}};
    static const std::unordered_map<std::string, int> majin_form_map = {{"affinity", 1},
                                                                        {"super",    2},
                                                                        {"true",     3}};

    static const std::map<int, transform_bonus> no_trans_bonus{};

    static const std::map<int, transform_bonus> human_trans_bonus = {
            {1, {.bonus=1000000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=12000000, .mult=3, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=50000000, .mult=4, .drain=.2, .flag=PLR_TRANS3}},
            {4, {.bonus=270000000, .mult=4.5, .drain=.2, .flag=PLR_TRANS4}},
    };

    static const std::map<int, transform_bonus> saiyan_trans_bonus = {
            {1, {.bonus=800000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=20000000, .mult=3, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=80000000, .mult=4, .drain=.2, .flag=PLR_TRANS3}},
            {4, {.bonus=182000000, .mult=5.5, .drain=.2, .flag=PLR_TRANS4}},
    };

    static const std::map<int, transform_bonus> namekian_trans_bonus = {
            {1, {.bonus=200000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=4000000, .mult=3, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=65000000, .mult=4, .drain=.2, .flag=PLR_TRANS3}},
            {4, {.bonus=230000000, .mult=4.5, .drain=.2, .flag=PLR_TRANS4}},
    };

    static const std::map<int, transform_bonus> konatsu_trans_bonus = {
            {1, {.bonus=1000000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=56000000, .mult=4, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=290000000, .mult=5, .drain=.2, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> icer_trans_bonus = {
            {1, {.bonus=400000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=7000000, .mult=3, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=45000000, .mult=4, .drain=.2, .flag=PLR_TRANS3}},
            {4, {.bonus=200000000, .mult=5, .drain=.2, .flag=PLR_TRANS4}},
    };


    static const std::map<int, transform_bonus> saiyan_trans_bonus_legendary = {
            {1, {.bonus=800000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=185000000, .mult=6, .drain=.2, .flag=PLR_TRANS2}},
    };

    static const std::map<int, transform_bonus> truffle_trans_bonus = {
            {1, {.bonus=1300000, .mult=3, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=80000000, .mult=4, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=300000000, .mult=5, .drain=.2, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> kai_trans_bonus = {
            {1, {.bonus=1100000, .mult=3, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=115000000, .mult=4, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=270000000, .mult=5, .drain=.2, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> halfbreed_trans_bonus = {
            {1, {.bonus=900000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=16500000, .mult=4, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=240000000, .mult=5, .drain=.2, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> mutant_trans_bonus = {
            {1, {.bonus=100000, .mult=2, .drain=.1, .flag=PLR_TRANS1}},
            {2, {.bonus=8500000, .mult=3, .drain=.2, .flag=PLR_TRANS2}},
            {3, {.bonus=80000000, .mult=4, .drain=.2, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> majin_trans_bonus = {
            {1, {.bonus=1250000, .mult=2, .drain=0, .flag=PLR_TRANS1}},
            {2, {.bonus=15000000, .mult=3, .drain=0, .flag=PLR_TRANS2}},
            {3, {.bonus=340000000, .mult=4.5, .drain=0, .flag=PLR_TRANS3}}
    };

    static const std::map<int, transform_bonus> bio_trans_bonus = {
            {1, {.bonus=1000000, .mult=2, .drain=0, .flag=PLR_TRANS1}},
            {2, {.bonus=8000000, .mult=3, .drain=0, .flag=PLR_TRANS2}},
            {3, {.bonus=70000000, .mult=3.5, .drain=0, .flag=PLR_TRANS3}},
            {4, {.bonus=400000000, .mult=4, .drain=0, .flag=PLR_TRANS4}},
    };

    static const std::map<int, transform_bonus> android_trans_bonus_sense = {
            {1, {.bonus=12500000, .mult=1, .drain=0, .flag=PLR_TRANS1}},
            {2, {.bonus=50000000, .mult=1, .drain=0, .flag=PLR_TRANS2}},
            {3, {.bonus=312000000, .mult=1, .drain=0, .flag=PLR_TRANS3}},
            {4, {.bonus=2500000000, .mult=1, .drain=0, .flag=PLR_TRANS4}},
            {5, {.bonus=5000000000, .mult=1, .drain=0, .flag=PLR_TRANS5}},
            {6, {.bonus=10000000000, .mult=1, .drain=0, .flag=PLR_TRANS6}},
    };

    static const std::map<int, transform_bonus> android_trans_bonus = {
            {1, {.bonus=5000000, .mult=1, .drain=0, .flag=PLR_TRANS1}},
            {2, {.bonus=20000000, .mult=1, .drain=0, .flag=PLR_TRANS2}},
            {3, {.bonus=125000000, .mult=1, .drain=0, .flag=PLR_TRANS3}},
            {4, {.bonus=1000000000, .mult=1, .drain=0, .flag=PLR_TRANS4}},
            {5, {.bonus=2500000000, .mult=1, .drain=0, .flag=PLR_TRANS5}},
            {6, {.bonus=5000000000, .mult=1, .drain=0, .flag=PLR_TRANS6}},
    };

    static const transform_bonus hoshi_birth = {
            .bonus=0, .mult=2, .drain=0, .flag=PLR_TRANS1
    };
    static const transform_bonus hoshi_life = {
            .bonus=0, .mult=3, .drain=0, .flag=PLR_TRANS2
    };

    const std::map<int, transform_bonus> &Race::getTransMap(const char_data *ch) const {
        switch (r_id) {
            case android:
                if (ch->playerFlags.test(PLR_SENSEM)) {
                    return android_trans_bonus_sense;
                } else {
                    return android_trans_bonus;
                }
            case saiyan:
                if (ch->playerFlags.test(PLR_LSSJ)) {
                    return saiyan_trans_bonus_legendary;
                } else {
                    return saiyan_trans_bonus;
                }
            case human:
                return human_trans_bonus;
            case namekian:
                return namekian_trans_bonus;
            case icer:
                return icer_trans_bonus;
            case konatsu:
                return konatsu_trans_bonus;
            case mutant:
                return mutant_trans_bonus;
            case halfbreed:
                return halfbreed_trans_bonus;
            case bio:
                return bio_trans_bonus;
            case majin:
                return majin_trans_bonus;
            case kai:
                return kai_trans_bonus;
            case truffle:
                return truffle_trans_bonus;
            default:
                return no_trans_bonus;
        }
    }


    const std::unordered_map<std::string, int> &Race::getTierMap(char_data *ch) const {
        switch (r_id) {
            case android:
                return android_form_map;
            case bio:
                return bio_form_map;
            case majin:
                return majin_form_map;
            default:
                switch (getMaxTransformTier(ch)) {
                    case 2:
                        return two_form_map;
                    case 3:
                        return three_form_map;
                    case 4:
                        return four_form_map;
                    default:
                        // This shouldn't actually be possible, but this is better than throwing an exception.
                        return no_form_map;
                }
        }
    }

    int Race::flagToTier(int flag) const {
        switch (flag) {
            case PLR_TRANS1:
                return 1;
            case PLR_TRANS2:
                return 2;
            case PLR_TRANS3:
                return 3;
            case PLR_TRANS4:
                return 4;
            case PLR_TRANS5:
                return 5;
            case PLR_TRANS6:
                return 6;
            default:
                return 0;
        }
    }

    transform_bonus Race::getCurForm(const char_data *ch) const {
        if (PLR_FLAGGED(ch, PLR_OOZARU)) return oozaru;
        if (IS_HOSHIJIN(ch)) {
            transform_bonus hoshi_form;
            double bon_mult = 0;
            switch (GET_PHASE(ch)) {
                case 0: // death phase
                    return base_form;
                case 1: // birth phase
                    hoshi_form = hoshi_birth;
                    bon_mult = 4;
                    break;
                case 2: // life phase
                    hoshi_form = hoshi_life;
                    bon_mult = 8;
                    break;
                default:
                    return base_form;
            }
            if (IN_ROOM(ch) != NOWHERE && ETHER_STREAM(ch))
                bon_mult += .5;
            hoshi_form.bonus = (ch->getBasePL() * .1) * bon_mult;
            return hoshi_form;
        }

        auto tier = getCurrentTransTier(ch);
        if (!tier) return base_form;
        auto t_map = getTransMap(ch);
        if (t_map.empty()) return base_form;
        return t_map[tier];
    }

    bool Race::raceCanBeSensed() const {
        switch (r_id) {
            case android:
                return false;
            default:
                return true;
        }
    }

    bool Race::raceHasNoisyTransformations() const {
        switch (r_id) {
            case android:
                return false;
            default:
                return true;
        }
    }

    std::optional<transform_bonus> Race::findForm(char_data *ch, const std::string &arg) const {
        auto trans_map = getTierMap(ch);
        auto trans_bonuses = getTransMap(ch);
        for (const auto &form: trans_map) {
            if (!strcasecmp(form.first.c_str(), arg.c_str())) {
                return trans_bonuses[form.second];
            }
        }
        return {};
    }

    int Race::getCurrentTransTier(const char_data *ch) const {
        int trans_tier = 0;

        for (const auto &flag: {PLR_TRANS1, PLR_TRANS2, PLR_TRANS3, PLR_TRANS4, PLR_TRANS5, PLR_TRANS6}) {
            trans_tier++;
            if (PLR_FLAGGED(ch, flag)) {
                return trans_tier;
            }
        }
        return 0;
    }


    bool Race::checkTransUnlock(char_data *ch, int tier) const {
        // First, check for special requirements which are not 'paid'.
        switch (r_id) {
            case bio:
                if (tier > 3 - GET_ABSORBS(ch)) {
                    send_to_char(ch, "You need to absorb something to transform!\r\n");
                    return false;
                }
                break;
            case majin:
                switch (tier) {
                    case 2:
                        if (GET_ABSORBS(ch) > 0) {
                            send_to_char(ch, "You need to ingest someone before you can use that form.\r\n");
                            return false;
                        }
                        if (GET_LEVEL(ch) < 50) {
                            send_to_char(ch, "You must be at least level 50 to reach that form.\r\n");
                            return false;
                        }
                }
                break;
        }
        int rpp_cost = 0;

        // Second, check for RPP requirements.
        switch (r_id) {
            case android:
                switch (tier) {
                    case 1:
                        break; // free for androids. They pay PS instead.
                    case 6:
                        rpp_cost = 5;
                        break;
                    default:
                        rpp_cost = 1;
                        break;
                }
                break;
            case majin:
                switch (tier) {
                    case 1:
                        rpp_cost = 1;
                        break;
                    case 2:
                    case 3:
                        rpp_cost = 2;
                        break;
                }
                break;

            default:
                if (getMaxTransformTier(ch) == tier) {
                    rpp_cost = 2;
                } else {
                    rpp_cost = 1;
                }
        }

        if (rpp_cost) {
            if (GET_TRANSCOST(ch, tier) == false) {
                if (GET_RP(ch) < rpp_cost) {
                    send_to_char(ch, "You need %i RPP in order to unlock this transformation.\r\n", rpp_cost);
                    return false;
                } else {
                    ch->modRPP(-rpp_cost);
                    GET_TRANSCOST(ch, tier) = true;
                    send_to_char(ch, "You pay %i RPP to permanently unlock this transformation!\r\n", rpp_cost);
                }
            }
        }

        // Android upgrades cost PS instead of RPP. But this system has now been standardized so anything can.
        int ps_cost = 0;
        switch (r_id) {
            case android:
                switch (tier) {
                    case 1:
                        ps_cost = 50;
                        break;
                }
        }

        if (ps_cost) {
            if (GET_TRANSCOST(ch, tier) == false) {
                if (GET_PRACTICES(ch) < 50) {
                    send_to_char(ch,
                                 "You need %i practice points in order to obtain a transformation for the first time.\r\n",
                                 ps_cost);
                    return false;
                } else {
                    ch->modPractices(-50);
                    GET_TRANSCOST(ch, tier) = true;
                    send_to_char(ch, "You pay %i PS to permanently unlock this transformation!\r\n", ps_cost);
                }
            }
        }

        // if we got down this far, we have unlocked the transformation!
        return true;

    }

    void Race::displayForms(char_data *ch) const {

        switch (r_id) {
            case human:
                send_to_char(ch, "              @YSuper @CHuman@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YSuper @CHuman @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CHuman @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CHuman @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CHuman @WFourth @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case saiyan:
                if (PLR_FLAGGED(ch, PLR_LSSJ)) {
                    send_to_char(ch, "                @YSuper @CSaiyan@n\r\n");
                    send_to_char(ch, "@b-------------------------------------------------@n\r\n");
                    send_to_char(ch, "@YSuper @CSaiyan @WFirst   @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@YLegendary @CSuper Saiyan @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@b-------------------------------------------------@n\r\n");
                } else {
                    send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
                    send_to_char(ch, "@b------------------------------------------------@n\r\n");
                    send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@YSuper @CSaiyan @WFourth @R-@G %s BPL Req\r\n",
                                 (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                                : "??????????");
                    send_to_char(ch, "@b------------------------------------------------@n\r\n");
                }
                break;
            case icer:
                send_to_char(ch, "              @YTransform@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YTransform @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YTransform @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YTransform @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YTransform @WFourth @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case konatsu:
                send_to_char(ch, "              @YShadow@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YShadow @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YShadow @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YShadow @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case namekian:
                send_to_char(ch, "              @YSuper @CNamek@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YSuper @CNamek @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CNamek @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CNamek @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CNamek @WFourth @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case mutant:
                send_to_char(ch, "              @YMutate@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YMutate @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMutate @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMutate @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case halfbreed:
                send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case bio:
                send_to_char(ch, "              @YPerfection@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YMature        @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSemi-Perfect  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YPerfect       @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YSuper Perfect @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case android:
                send_to_char(ch, "              @YUpgrade@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@Y1.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@Y2.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@Y3.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@Y4.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@Y5.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 5) * 0.75) ? add_commas(trans_req(ch, 5)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@Y6.0 @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 6) * 0.75) ? add_commas(trans_req(ch, 6)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case majin:
                send_to_char(ch, "              @YMorph@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YMorph @WAffinity @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMorph @WSuper    @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMorph @WTrue     @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case kai:
                send_to_char(ch, "              @YMystic@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YMystic @WFirst     @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMystic @WSecond    @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YMystic @WThird     @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            case truffle:
                send_to_char(ch, "              @YAscend@n\r\n");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                send_to_char(ch, "@YAscend @WFirst  @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YAscend @WSecond @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@YAscend @WThird @R-@G %s BPL Req\r\n",
                             (ch->getBasePL()) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)).c_str()
                                                                            : "??????????");
                send_to_char(ch, "@b------------------------------------------------@n\r\n");
                break;
            default:
                send_to_char(ch, "You do not have a transformation.\r\n");
                break;
        }
    }

    void Race::displayTransReq(char_data *ch) const {

        switch (GET_TRANSCLASS(ch)) {
            case 1:
                send_to_char(ch, "\r\n@RYou have @rterrible@R transformation BPL Requirements.@n\r\n");
                break;
            case 2:
                send_to_char(ch, "\r\n@CYou have @caverage@C transformation BPL Requirements.@n\r\n");
                break;
            case 3:
                send_to_char(ch, "\r\n@GYou have @gGREAT@G transformation BPL Requirements.@n\r\n");
                break;
        }
    }

    void Race::echoTransform(char_data *ch, int tier) const {
        switch (r_id) {
            case human:
                switch (tier) {
                    case 1:
                        act("@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WSuddenly a bright white aura bursts into existance around your body, you feel the intensity of your hidden potential boil until it can't be contained any longer! Waves of ki shoot out from your aura streaking outwards in many directions. A roar that shakes everything in the surrounding area sounds right as your energy reaches its potential and you achieve @CSuper @cHuman @GSecond@W!",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W is suddenly covered with a bright white aura as $e grits $s teeth, apparently struggling with the power boiling to the surface! Waves of ki shoot out from $s aura, streaking in several directions as a mighty roar shakes everything in the surrounding area. As $s aura calms $e smiles, having achieved @CSuper @cHuman @GSecond@W!",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WYou clench both of your fists as the bright white aura around your body is absorbed back into your flesh. As it is absorbed, your muscles triple in size and electricity crackles across your flesh. You grin as you feel the power of @CSuper @cHuman @GThird@W!",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W clenches both of $s fists as the bright white aura around $s body is absorbed back into $s flesh. As it is absorbed, $s muscles triple in size and bright electricity crackles across $s flesh. $e smiles as $e achieves the power of @CSuper @cHuman @GThird@W!",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WYou grit your teeth and clench your fists as a sudden surge of power begins to tear through your body! Your muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over you, flashes of multicolored light flaring up in rising stars around your new form, a corona of glory! You feel your ultimate potential realized as you ascend to @CSuper @cHuman @GFourth@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W grits $s teeth and clenches $s fists as a sudden surge of power begins to tear through $s body! $n@W's muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over $m, flashes of multicolored light flare up in rising stars around $s new form, a corona of glory! $n@W smiles as his ultimate potential is realized as $e ascends to @CSuper @cHuman @GFourth@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    default:
                        return;
                }
            case icer:
                switch (tier) {
                    case 1:
                        act("@WYou yell with pain as your body begins to grow and power surges within! Your legs expand outward to triple their previous length. Soon after your arms, chest, and head follow. Your horns grow longer and curve upwards while lastly your tail expands. You are left confidently standing, having completed your @GFirst @cTransformation@W.@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W yells with pain as $s body begins to grow and power surges outward! $s legs expand outward to triple their previous length. Soon after $s arms, chest, and head follow. $s horns grow longer and curve upwards while lastly $s tail expands. $e is left confidently standing, having completed $s @GFirst @cTransformation@W.@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WA blinding light surrounds your body while your rising power begins to rip up the ground beneath you! Your skin and torso shell begin to crack as your new body struggles to free its self. Huge chunks of debris lift free of the ground as your power begins to rise to unbelievable heights. Suddenly your old skin and torso shell burst off from your body, leaving a sleek form glowing where they had been. Everything comes crashing down as your power evens out, leaving you with your @GThird @cTransformation @Wcompleted!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA blinding light surrounds @C$n@W's body while $s rising power begins to rip up the ground beneath $m! $s skin and torso shell begin to crack as $s new body struggles to free its self. Huge chunks of debris lift free of the ground as $s power begins to rise to unbelievable heights. Suddenly $s old skin and torso shell burst off from $s body, leaving a sleek form glowing where they had been. Everything comes crashing down as @C$n@W's power evens out, leaving $m with $s @GThird @cTransformation @Wcompleted!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WA feeling of complete power courses through your viens as your body begins to change radically! You triple in height while a hard shell forms over your entire torso. Hard bones grow out from your head forming four ridges that jut outward. A hard covering grows up over your mouth and nose completing the transformation! A dark crimson aura flames around your body as you realize your @GFourth @cTransformation@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W's body begins to change radically! $e triples in height while a hard shell forms over $s entire torso. Hard bones grow out from $s head forming four ridges that jut outward. A hard covering grows up over $s mouth and nose completing the transformation! A dark crimson aura flames around @C$n@W's body as $e realizes $s @GFourth @cTransformation@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case konatsu:
                switch (tier) {
                    case 1:
                        act("@WA dark shadowy aura with flecks of white energy begins to burn around your body! Strength and agility can be felt rising up within as your form becomes blurred and ethereal looking. You smile as you realize your @GFirst @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA dark shadowy aura with flecks of white energy begins to burn around @C$n@W's body! $s form becomes blurred and ethereal-looking as $s muscles become strong and lithe. $e smiles as $e achieves $s @GFirst @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WThe shadowy aura surrounding your body burns larger than ever as dark bolts of purple electricity crackles across your skin. Your eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into your aura in an instant as you achieve your @GSecond @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WThe shadowy aura surrounding @C$n@W's body burns larger than ever as dark bolts of purple electricity crackles across $s skin. $s eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into $s aura in an instant as $e achieves $s @GSecond @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WThe shadowy aura around you explodes outward as your power begins to rise!  You're overcome with a sudden realization, that the shadows are an extension of yourself, that light isn't needed for your shadows to bloom.  With this newfound wisdom comes ability and power!  The color in your aura drains as the shadows slide inward and cling to your body like a second, solid black skin!  Shockwaves roll off of you in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath you, slowly spreading the shadows to the whole area, projecting onto any surface nearby!  Purple and black electricity crackle in your solid white aura, and you grin as you realize your @GThird @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WThe shadowy aura around $n explodes outward as $s power begins to rise!  Realization dawns on $s face, followed shortly by confidence! The color in $s aura drains as the shadows slide inward to cling to $s body like a second, solid black skin! Shockwaves roll off of $n in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath them, slowly spreading the shadows to the whole area, projecting onto any surface nearby! Purple and black electricity crackle in $s solid white aura, and he grins as $e realizes $s @GThird @DShadow @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case namekian:
                switch (tier) {
                    case 1:
                        act("@WYou crouch down and clench your fists as your muscles begin to bulge! Sweat pours down your body as the ground beneath your feet cracks and warps under the pressure of your rising ki! With a sudden burst that sends debris flying you realize a new plateau in your power, having achieved @CSuper @gNamek @GFirst@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wcrouches down and clenches $s fists as $s muscles begin to bulge! Sweat pours down $s body as the ground beneath $s feet cracks and warps under the pressure of  $s rising ki! With a sudden burst that sends debris flying $e seems to realize a new plateau in $s power, having achieved @CSuper @gNamek @GFirst@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WYou gasp in shock as a power within your body that you had not been aware of begins to surge to the surface! Your muscles grow larger as energy crackles between your antennae intensely! A shockwave of energy explodes outward as you achieve a new plateau in power, @CSuper @gNamek @GSecond@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wgasps in shock as a power within $s body begins to surge out! $s muscles grow larger as energy crackles between $s antennae intensely! A shockwave of energy explodes outward as $e achieves a new plateau in power, @CSuper @gNamek @GSecond@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WA fierce clear aura bursts up around your body as you struggle to control a growing power within! Energy leaks off of your aura at an astounding rate filling the air around you with small orbs of ki. As your power begins to level off the ambient ki hovering around you is absorbed inward in a sudden shock that leaves your skin glowing! You have achieved a rare power, @CSuper @gNamek @GThird@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA fierce clear aura bursts up around @C$n@W's body as $e struggles to control $s own power! Energy leaks off of $s aura at an astounding rate filling the air around $m with small orbs of ki. As $s power begins to level off the ambient ki hovering around $m is absorbed inward in a sudden shock that leaves $s skin glowing! $e has achieved a rare power, @CSuper @gNamek @GThird@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WAn inner calm fills your mind as your power surges higher than ever before. Complete clarity puts everything once questioned into perspective. While this inner calm is filling your mind, an outer storm of energy erupts around your body! The storm of energy boils and crackles while growing larger. You have achieved @CSuper @gNamek @GFourth@W, a mystery of the ages.@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W smiles calmly as a look of complete understand fills $s eyes. While $e remains perfectly calm and detached a massivly powerful storm of energy erupts from his body. This storm of energy shimmers with the colors of the rainbow and boils and crackles with awesome power! $s smile disappears as he realizes a mysterious power of the ages, @CSuper @gNamek @GFourth@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case mutant:
                switch (tier) {
                    case 1:
                        act("@WYour flesh grows tougher as power surges up from within. Your fingernails grow longer, sharper, and more claw-like. Lastly your muscles double in size as you achieve your @GFirst @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W flesh grows tougher as power surges up around $m. $s fingernails grow longer, sharper, and more claw-like. Lastly $s muscles double in size as $e achieves $s @GFirst @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WA dark cyan aura bursts up around your body as the ground begins to crack beneath you! You scream out in pain as your power begins to explode! Two large spikes grow out from your shoulder blades as you reach your @GThird @mMutation!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA dark cyan aura bursts up around @C$n@W's body as the ground begins to crack beneath $m and $e screams out in pain as $s power begins to explode! Two large spikes grow out from $s shoulder blades as $e reaches $s @GThird @mMutation!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case halfbreed:
            case saiyan:
                switch (tier) {
                    case 1:
                        act("@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        if (PLR_FLAGGED(ch, PLR_LSSJ)) {
                            act("@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n",
                                true, ch, nullptr, nullptr, TO_CHAR);
                            act("@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n",
                                true, ch, nullptr, nullptr, TO_ROOM);
                            return;
                        } else {
                            act("@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n",
                                true, ch, nullptr, nullptr, TO_CHAR);
                            act("@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n",
                                true, ch, nullptr, nullptr, TO_ROOM);
                        }
                        return;
                    case 3:
                        act("@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WHaving absorbed enough blutz waves, your body begins to transform! Red fur grows over certain parts of your skin as your hair grows longer and unkempt. A red outline forms around your eyes while the irises of those very same eyes change to an amber color. Energy crackles about your body violently as you achieve the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WHaving absorbed enough blutz waves, @C$n@W's body begins to transform! Red fur grows over certain parts of $s skin as $s hair grows longer and unkempt. A red outline forms around $s eyes while the irises of those very same eyes change to an amber color. Energy crackles about $s body violently as $e achieves the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case bio:
                switch (tier) {
                    case 1:
                        act("@gYou bend over as @rpain@g wracks your body! Your limbs begin to grow out, becoming more defined and muscular. As your limbs finish growing outward you feel a painful sensation coming from your back as a long tail with a spike grows out of your back! As the pain subsides you stand up straight and a current of power shatters part of the ground beneath you. You have @rmatured@g beyond your @Gl@ga@Dr@gv@Ga@ge stage!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@W$n @gbends over as a @rpainful@g look covers $s face! $s limbs begin to grow out, becoming more defined and muscular. As $s limbs finish growing outward $e screams as a long tail with a spike grows rips out of $s back! As $e calms $e stands up straight and a current of power shatters part of the ground beneath $m. $e has @rmatured@g beyond $s @Gl@ga@Dr@gv@Ga@ge stage!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WYour exoskeleton begins to glow spectacularly while the shape of your body begins to change. Your tail shrinks slightly. Your hands, feet, and facial features become more refined. While your body colors change slightly. The crests on your head change, standing up straighter on either side of your head as well. As you finish transforming a wave of power floods your being. You have achieved your @gSemi@D-@GPerfect @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W's exoskeleton begins to glow spectacularly while the shape of $s body begins to change. $s tail shrinks slightly. $s hands, feet, and facial features become more refined. While $s body colors change slightly. The crests on $s head change, standing up straighter on either side of $s head as well. As $e finishes transforming a wave of power rushes out from $m. $e has achieved $s @gSemi@D-@GPerfect @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WYour whole body is engulfed in blinding light as your exoskeleton begins to change shape! Your hands, feet, and facial features become more refined and humanoid. While your colors change, becoming more subdued and neutral. A bright golden aura bursts up around your body as you achieve your @GPerfect @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W whole body is engulfed in blinding light as $s exoskeleton begins to change shape! $s hands, feet, and facial features become more refined and humanoid. While $s colors change, becoming more subdued and neutral. A bright golden aura bursts up around $s body as $e achieves $s @GPerfect @BForm@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WA rush of power explodes from your perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout your aura intensely while your muscles grow slightly larger but incredibly dense. You smile as you realize that you have taken your perfect form beyond imagination. You are now @CSuper @GPerfect@W!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA rush of power explodes from @C$n@W's perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout $s aura intensely while $s muscles grow slightly larger but incredibly dense. $e smiles as $e has taken $s perfect form beyond imagination. $e is now @CSuper @GPerfect@W!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case android:
                switch (tier) {
                    case 1:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 5:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 6:
                        act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case majin:
                switch (tier) {
                    case 1:
                        act("@WA dark pink aura bursts up around your body as images of good and evil fill your mind! You feel the power within your body growing intensely, reflecting your personal alignment as your body changes!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WA dark pink aura bursts up around @C$n@W's body as images of good and evil fill $s mind! $e feels the power within $s body growing intensely, reflecting $s personal alignment as $s body changes!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WAn intense pink aura surrounds your body as it begins to change, taking on the characteristics of those you have ingested! Explosions of pink energy burst into existence all around you as your power soars to sights unseen!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WAn intense pink aura surrounds @C$n@W's body as it begins to change, taking on the characteristics of those $e has ingested! Explosions of pink energy burst into existence all around $m as $s power soars to sights unseen!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WRipples of intense pink energy rush upwards around your body as it begins to morph into its truest form! The ground beneath your feet forms into a crater from the very pressure of your rising ki! Earthquakes shudder throughout the area as your finish morphing!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@WRipples of intense pink energy rush upwards around @C$n@W's body as it begins to morph into its truest form! The ground beneath $s feet forms into a crater from the very pressure of $s rising ki! Earthquakes shudder throughout the area as $e finishes morphing!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case kai:
                switch (tier) {
                    case 1:
                        act("@WThoughts begin to flow through your mind of events throughout your life. The progression leads up to more recent events and finally to this very moment. All of it's significance overwhelms you momentarily and your motivation and drive increase. As your attention is drawn back to your surroundings, you feel as though your thinking, senses, and reflexes have sharpened dramatically.  At the core of your being, a greater depth of power can be felt.@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@W$n@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings. Though it's not apparent why they were so distracted, something definitely seems different about $m.@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WYou feel a sudden rush of emotion, escalating almost to a loss of control as your thoughts race. Your heart begins to beat fast as memories mix with the raw emotion. A faint blue glow begins to surround you. As your emotions level off, you feel a deeper understanding of the universe as you know it. You visibly calm back down to an almost steely eyed resolve as you assess your surroundings. The blue aura wicks around you for a few moments and then dissipates. Thought it's full impact is not yet clear to you, you are left feeling as though both your power and inner strength have turned into nearly bottomless wells.@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@W$n@W's appears to be hit by some sudden pangs of agony, their face contorted in pain.  After a moment a faint blue aura appears around them, glowing brighter as time passes. You can feel something in the pit of your stomach, letting you know that something very significant is changing around you. Before long $n@W's aura fades, leaving a very determined looking person in your presence.@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WYour minds' eye becomes overwhelmed by secrets unimaginable. The threads of the very universe become visible in your heightened state of awareness. Reaching out, a single thread vibrates, producing a @Rred @Wcolor -- yours. Your fingertips brush against it and your senses become clouded by a vast expanse of white color and noise. As your vision and hearing return, you understand the threads tying every living being together. Your awareness has expanded beyond comprehension!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W's eyes grow wide, mouth agape. $s body begins to shiver uncontrollably! $s arms reaches out cautiously before falling back down to $s side. $s face relaxes visibly, features returning to a normal state. $s irises remain larger than before, a slight smile softening $s gaze.@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case truffle:
                switch (tier) {
                    case 1:
                        act("@WYour mind accelerates working through the mysteries of the universe while at the same time your body begins to change! Innate nano-technology within your body begins to activate, forming flexible metal plating across parts of your skin!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@W begins to write complicated calculations in the air as though $e were possessed while at the same time $s body begins to change! Innate nano-technology within $s body begins to activate, forming flexible metal plating across parts of $s skin!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@WComplete understanding of every physical thing floods your mind as the nano-technology within you continues to change your body! Your eyes change; becoming glassy, hard, and glowing. Your muscles merge with a nano-fiber strengthening them at the molecular level! Finally your very bones become plated in nano-metals that have yet to be invented naturally!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n@.s nano-technology continues to change $s body! $s eyes change; becoming glassy, hard, and glowing. $s muscles merge with a nano-fiber strengthening them at the molecular level! Finally $s very bones become plated in nano-metals that have yet to be invented naturally!@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@WYou have reached the final stage of enlightenment and the nano-technology thriving inside you begin to initiate the changes! Your neural pathways become refined, your reflexes honed, your auditory and ocular senses sharpening far beyond normal levels! Your gravitational awareness improves, increasing sensitivity and accuracy in your equilibrum!@n",
                            true, ch, nullptr, nullptr, TO_CHAR);
                        act("@C$n begins to mumble quietly, slowly at first and gradually picking up speed. A glint is seen from $s eyes and $s arms reach outwards briefly as $e appears to catch his balance. $s arms drop back to $s sides as balance is regained, a vicious smile on $s face.@n",
                            true, ch, nullptr, nullptr, TO_ROOM);
                        return;

                }
        }
        send_to_char(ch,
                     "You strain and scream and... hold on, you don't have a transformation. How did this happen? Go tell an admin!\r\n");
        return;
    }

    void Race::echoRevert(char_data *ch, int tier) const {
        switch (r_id) {
            case human:
                switch (tier) {
                    case 1:
                        act("@wYou revert from @CSuper @cHuman @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cHuman @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CSuper @cHuman @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cHuman @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@wYou revert from @CSuper @cHuman @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cHuman @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 4:
                        act("@wYou revert from @CSuper @cHuman @GFourth@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cHuman @GFourth@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
                break;
            case saiyan:
            case halfbreed:
                switch (tier) {
                    case 4:
                        act("@wYou revert from @CSuper @cSaiyan @GFourth@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cSaiyan @GFourth@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@wYou revert from @CSuper @cSaiyan @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cSaiyan @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        if (PLR_FLAGGED(ch, PLR_LSSJ)) {
                            act("@wYou revert from your @YLegendary @CSuper Saiyan@w form.@n", true, ch, nullptr,
                                nullptr, TO_CHAR);
                            act("@w$n@w reverts from $s @YLegendary @CSuper Saiyan@w form@w.@n", true, ch, nullptr,
                                nullptr,
                                TO_ROOM);
                        } else {
                            act("@wYou revert from @CSuper @cSaiyan @GSecond@w.@n", true, ch, nullptr, nullptr,
                                TO_CHAR);
                            act("@w$n@w reverts from @CSuper @cSaiyan @GSecond@w.@n", true, ch, nullptr, nullptr,
                                TO_ROOM);
                        }
                        return;
                    case 1:
                        act("@wYou revert from @CSuper @cSaiyan @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cSaiyan @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case icer:
                switch (tier) {
                    case 4:
                        act("@wYou revert from @CTransform @GFourth@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CTransform @GFourth@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@wYou revert from @CTransform @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CTransform @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CTransform @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CTransform @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 1:
                        act("@wYou revert from @CTransform @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CTransform @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case konatsu:
                switch (tier) {
                    case 3:
                        act("@wYou revert from @CShadow @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CShadow @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CShadow @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CShadow @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 1:
                        act("@wYou revert from @CShadow @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CShadow @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case namekian:
                switch (tier) {
                    case 4:
                        act("@wYou revert from @CSuper @cNamek @GFourth@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cNamek @GFourth@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 3:
                        act("@wYou revert from @CSuper @cNamek @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cNamek @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CSuper @cNamek @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cNamek @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 1:
                        act("@wYou revert from @CSuper @cNamek @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CSuper @cNamek @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
                break;
            case mutant:
                switch (tier) {
                    case 3:
                        act("@wYou revert from @CMutate @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMutate @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CMutate @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMutate @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 1:
                        act("@wYou revert from @CMutate @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMutate @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
            case kai:
                switch (tier) {
                    case 3:
                        act("@wYou revert from @CMystic @GThird@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMystic @GThird@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 2:
                        act("@wYou revert from @CMystic @GSecond@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMystic @GSecond@w.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                    case 1:
                        act("@wYou revert from @CMystic @GFirst@w.@n", true, ch, nullptr, nullptr, TO_CHAR);
                        act("@w$n@w reverts from @CMystic @GFirst.@n", true, ch, nullptr, nullptr, TO_ROOM);
                        return;
                }
        }
    }

    int Race::getRPPRefund() const {
        switch (r_id) {
            case majin:
                return 35;
            case hoshijin:
                return 15;
            case saiyan:
                return 40;
            case bio:
                return 20;
            default:
                return 0;
        }
    }

    bool Race::raceIsPeople() const {
        switch (r_id) {
            case animal:
            case saiba:
            case mechanical:
            case spirit:
                return false;
            default:
                return true;
        }
    }

    bool Race::raceHasTail() const {
        switch (r_id) {
            case icer:
            case bio:
            case saiyan:
            case halfbreed:
                return true;
            default:
                return false;
        }
    }

    bool Race::hasTail(char_data *ch) const {
        if (!raceHasTail())
            return false;
        switch (r_id) {
            case icer:
            case bio:
                return PLR_FLAGGED(ch, PLR_TAIL);
            case saiyan:
            case halfbreed:
                return PLR_FLAGGED(ch, PLR_STAIL);
            default:
                return false;
        }
    }

    void Race::loseTail(char_data *ch) const {
        if (!hasTail(ch)) return;
        switch (r_id) {
            case icer:
            case bio:
                ch->playerFlags.reset(PLR_TAIL);
                remove_limb(ch, 6);
                GET_TGROWTH(ch) = 0;
                break;
            case saiyan:
            case halfbreed:
                ch->playerFlags.reset(PLR_STAIL);
                remove_limb(ch, 5);
                if (PLR_FLAGGED(ch, PLR_OOZARU)) {
                    oozaru_revert(ch);
                }
                GET_TGROWTH(ch) = 0;
                break;
        }
    }

    void Race::gainTail(char_data *ch, bool announce) const {
        if (hasTail(ch)) return;
        switch (r_id) {
            case icer:
            case bio:
                ch->playerFlags.set(PLR_TAIL);
                break;
            case saiyan:
            case halfbreed:
                ch->playerFlags.set(PLR_STAIL);
                if (MOON_OK(ch)) {
                    oozaru_transform(ch);
                }
                break;
        }
    }

    static std::map<int, int64_t> soft_cap_variable = {
            {0, 1500},
            {1, 4500},
            {2, 15000},
            {3, 45000},
            {4, 60000},
            {5, 240000},
            {6, 600000},
            {7, 750000},
            {8, 2400000},
            {9, 4500000}
    };

    static std::map<int, int64_t> soft_cap_fixed = {
            {0, 500},
            {1, 1500},
            {2, 5000},
            {3, 15000},
            {4, 20000},
            {5, 80000},
            {6, 200000},
            {7, 250000},
            {8, 800000},
            {9, 1500000}
    };

    static std::map<int, int64_t> soft_cap_demon = {
            {0, 500},
            {1, 1500},
            {2, 5000},
            {3, 25000},
            {4, 40000},
            {5, 100000},
            {6, 250000},
            {7, 300000},
            {8, 1000000},
            {9, 2000000}
    };

    const std::map<int, int64_t> &Race::getSoftMap(const char_data *ch) const {
        switch (r_id) {
            case demon:
            case kanassan:
                return soft_cap_demon;
            default:
                return soft_cap_fixed;
        }
    }

    SoftCapType Race::getSoftType(const char_data *ch) const {
        return Fixed;
    }

    RaceMap race_map;

    RaceMap valid_for_sex(int sex) {
        RaceMap v_map;
        std::copy_if(race::race_map.begin(), race::race_map.end(), std::inserter(v_map, begin(v_map)),
                     [&](const auto &r) {
                         return r.second->isValidSex(sex);
                     });
        return v_map;
    }

    RaceMap valid_for_sex_pc(int sex) {
        RaceMap v_map;
        std::copy_if(race::race_map.begin(), race::race_map.end(), std::inserter(v_map, begin(v_map)),
                     [&](const auto &r) {
                         return r.second->isPcOk() && r.second->isValidSex(sex);
                     });
        return v_map;
    }

    Race *find_race(const std::string &arg) {
        return find_race_map(arg, race_map);
    }

    Race *find_pc_race(const std::string &arg) {
        RaceMap r_map;
        for (const auto &r: race_map) {
            if (r.second->isPcOk()) {
                r_map[r.first] = r.second;
            }
        }
        return find_race_map(arg, r_map);;
    }

    Race *find_race_map(const std::string &arg, const RaceMap &r_map) {
        std::string lower(arg);
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (const auto &r: r_map) {
            if (r.second->getNameLower() == lower) {
                return r.second;
            }
        }
        return nullptr;
    }

    Race *find_race_map_id(const int id, const RaceMap &r_map) {
        for (const auto &r: r_map) {
            if (r.first == id) {
                return r.second;
            }
        }
        return nullptr;
    }

    void load_races() {
        if (!race_map.empty())
            return;
        race_map[race_id::human] = new Race(race_id::human, "Human", "Hum", SIZE_MEDIUM, true);
        race_map[race_id::saiyan] = new Race(race_id::saiyan, "Saiyan", "Sai", SIZE_MEDIUM, true);
        race_map[race_id::icer] = new Race(race_id::icer, "Icer", "Ice", SIZE_MEDIUM, true);
        race_map[race_id::konatsu] = new Race(race_id::konatsu, "Konatsu", "kon", SIZE_MEDIUM, true);
        race_map[race_id::namekian] = new Race(race_id::namekian, "Namekian", "Nam", SIZE_MEDIUM, true);
        race_map[race_id::mutant] = new Race(race_id::mutant, "Mutant", "Mut", SIZE_MEDIUM, true);
        race_map[race_id::kanassan] = new Race(race_id::kanassan, "Kanassan", "Kan", SIZE_MEDIUM, true);
        race_map[race_id::halfbreed] = new Race(race_id::halfbreed, "Halfbreed", "H-B", SIZE_MEDIUM, true);
        race_map[race_id::bio] = new Race(race_id::bio, "BioAndroid", "Bio", SIZE_MEDIUM, true);
        race_map[race_id::android] = new Race(race_id::android, "Android", "And", SIZE_MEDIUM, true);
        race_map[race_id::demon] = new Race(race_id::demon, "Demon", "Dem", SIZE_MEDIUM, true);
        race_map[race_id::majin] = new Race(race_id::majin, "Majin", "Maj", SIZE_MEDIUM, true);
        race_map[race_id::kai] = new Race(race_id::kai, "Kai", "Kai", SIZE_MEDIUM, true);
        race_map[race_id::truffle] = new Race(race_id::truffle, "Truffle", "Tru", SIZE_SMALL, true);
        race_map[race_id::hoshijin] = new Race(race_id::hoshijin, "Hoshijin", "Hos", SIZE_MEDIUM, true);
        race_map[race_id::animal] = new Race(race_id::animal, "Animal", "Ict", SIZE_FINE, false);
        race_map[race_id::saiba] = new Race(race_id::saiba, "Saiba", "Sab", SIZE_LARGE, false);
        race_map[race_id::serpent] = new Race(race_id::serpent, "Serpent", "Ser", SIZE_MEDIUM, false);
        race_map[race_id::ogre] = new Race(race_id::ogre, "Ogre", "Ogr", SIZE_LARGE, false);
        race_map[race_id::yardratian] = new Race(race_id::yardratian, "Yardratian", "Yar", SIZE_MEDIUM, false);
        race_map[race_id::arlian] = new Race(race_id::arlian, "Arlian", "Arl", SIZE_MEDIUM, true);
        race_map[race_id::dragon] = new Race(race_id::dragon, "Dragon", "Drg", SIZE_MEDIUM, false);
        race_map[race_id::mechanical] = new Race(race_id::mechanical, "Mechanical", "Mec", SIZE_MEDIUM, false);
        race_map[race_id::spirit] = new Race(race_id::spirit, "Spirit", "Spi", SIZE_TINY, false);

    }



}
