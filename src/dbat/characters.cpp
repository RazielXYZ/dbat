//
// Created by basti on 10/24/2021.
//
#include "dbat/structs.h"
#include "dbat/races.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/config.h"
#include "dbat/mail.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/players.h"

static std::string robot = "Robotic-Humanoid", robot_lower = "robotic-humanoid", unknown = "UNKNOWN";


const std::string &char_data::juggleRaceName(bool capitalized) {
    if (!race) return unknown;

    race::Race *apparent = race;

    switch (apparent->getID()) {
        case race::hoshijin:
            if (mimic) apparent = mimic;
            break;
        case race::halfbreed:
            switch ((int)RACIAL_PREF(this)) {
                case 1:
                    apparent = race::race_map[race::human];
                    break;
                case 2:
                    apparent = race::race_map[race::saiyan];
                    break;
            }
            break;
        case race::android:
            switch ((int)RACIAL_PREF(this)) {
                case 1:
                    apparent = race::race_map[race::android];
                    break;
                case 2:
                    apparent = race::race_map[race::human];
                    break;
                case 3:
                    if (capitalized) {
                        return robot;
                    } else {
                        return robot_lower;
                    }
            }
            break;
        case race::saiyan:
            if (PLR_FLAGGED(this, PLR_TAILHIDE)) {
                apparent = race::race_map[race::human];
            }
            break;
    }

    if (capitalized) {
        return apparent->getName();
    } else {
        return apparent->getNameLower();
    }
}

void char_data::restore_by(char_data *ch) {
    this->restore(true);

    ::act("You have been fully healed by $N!", false, this, nullptr, ch, TO_CHAR | TO_SLEEP);
}

void char_data::restore(bool announce) {
    restoreVitals(announce);
    restoreLimbs(announce);
    restoreStatus(announce);
    restoreLF(announce);
}

void char_data::resurrect(ResurrectionMode mode) {
    // First, fully heal the character.
    restore(true);
    for(auto f : {AFF_ETHEREAL, AFF_SPIRIT}) affected_by.reset(f);
    playerFlags.reset(PLR_PDEATH);
    // Send them to their starting room and have them 'look'.
    char_from_room(this);
    if (GET_DROOM(this) != NOWHERE && GET_DROOM(this) != 0 && GET_DROOM(this) != 1) {
        char_to_room(this, real_room(GET_DROOM(this)));
    } else {
        char_to_room(this, real_room(this->chclass->senseiStartRoom()));
    }
    look_at_room(in_room, this, 0);

    // If Costless, there's not going to be any penalties.
    int dur = 100;
    switch (mode) {
        case Costless:
            return;
        case Basic:
            if (dcount >= 8 && dcount < 10) {
                dur = 90;
            } else if (dcount >= 5 && dcount < 8) {
                dur = 75;
            } else if (dcount >= 3 && dcount < 5) {
                dur = 60;
            } else if (dcount >= 1 && dcount < 3) {
                dur = 40;
            }
            break;
        case RPP:
            dur = 100;
            break;
    }

    // Also no penalties if the character isn't at least level 10.
    if (GET_LEVEL(this) > 9) {
        int losschance = axion_dice(0);
        send_to_char(this,
                     "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength,itution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        std::map<CharAttribute, int> statReductions = {
            {CharAttribute::Strength, -8},
            {CharAttribute::Constitution, -8},
            {CharAttribute::Intelligence, -8},
            {CharAttribute::Wisdom, -8},
            {CharAttribute::Speed, -8},
            {CharAttribute::Agility, -8}
        };

        for (auto& [attr, reduction] : statReductions) {
            int baseStat = this->get(attr, true);
            int effectiveReduction = std::max(reduction, 15 - baseStat);
            statReductions[attr] = effectiveReduction;
        }

        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur,
                      statReductions[CharAttribute::Strength],
                      statReductions[CharAttribute::Constitution],
                      statReductions[CharAttribute::Intelligence],
                      statReductions[CharAttribute::Agility],
                      statReductions[CharAttribute::Wisdom],
                      statReductions[CharAttribute::Speed]);

        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            modPractices(-psloss);
            send_to_char(this, "@R...and a loss of @r%d@R PS!@n", psloss);
        }
    }
    GET_DTIME(this) = 0;
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void char_data::ghostify() {
    restore(true);
    for(auto f : {AFF_SPIRIT, AFF_ETHEREAL, AFF_KNOCKED, AFF_SLEEP, AFF_PARALYZE}) affected_by.reset(f);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP)) {
        for(auto f : {PLR_CLLEG, PLR_CRLEG, PLR_CLARM, PLR_CRARM}) playerFlags.reset(f);
    }

}

void char_data::teleport_to(IDXTYPE rnum) {
    char_from_room(this);
    char_to_room(this, real_room(rnum));
    look_at_room(IN_ROOM(this), this, 0);
    update_pos(this);
}

bool char_data::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return GET_ROOM_VNUM(IN_ROOM(this)) >= low_rnum && GET_ROOM_VNUM(IN_ROOM(this)) <= high_rnum;
}

bool char_data::in_past() {
    return ROOM_FLAGGED(IN_ROOM(this), ROOM_PAST);
}

bool char_data::is_newbie() {
    return GET_LEVEL(this) < 9;
}

bool char_data::in_northran() {
    return in_room_range(17900, 17999);
}

static std::map<int, uint16_t> grav_threshold = {
        {10,    5000},
        {20,    20000},
        {30,    50000},
        {40,    100000},
        {50,    200000},
        {100,   400000},
        {200,   1000000},
        {300,   5000000},
        {400,   8000000},
        {500,   15000000},
        {1000,  25000000},
        {5000,  100000000},
        {10000, 200000000}
};

bool char_data::can_tolerate_gravity(int grav) {
    if (IS_NPC(this)) return true;
    int tolerance = 0;
    tolerance = std::max(tolerance, this->chclass->getGravTolerance());
    if (tolerance >= grav)
        return true;
    return GET_MAX_HIT(this) >= grav_threshold[grav];
}


int char_data::calcTier() {
    auto level = get(CharNum::Level);
    int tier = level / 10;
    if ((level % 10) == 0)
        tier--;
    tier = std::max(tier, 0);
    tier = std::min(tier, 9);
    return tier;
}

int64_t char_data::calc_soft_cap() {
    auto level = get(CharNum::Level);
    if(level >= 100) return 50e9;
    auto tier = calcTier();
    auto softmap = race->getSoftMap(this);
    return level * softmap[tier];
}

bool char_data::is_soft_cap(int64_t type) {
    return is_soft_cap(type, 1.0);
}

bool char_data::is_soft_cap(int64_t type, long double mult) {
    if (IS_NPC(this))
        return true;

    // Level 100 characters are never softcapped.
    if (get(CharNum::Level) >= 100) {
        return false;
    }
    auto cur_cap = calc_soft_cap() * mult;

    int64_t against = 0;

    switch (type) {
        case 0:
            against = (getBasePL());
            break;
        case 1:
            against = (getBaseKI());
            break;
        case 2:
            against = (getBaseST());
            break;
    }

    return against >= cur_cap;
}

int char_data::wearing_android_canister() {
    if (!IS_ANDROID(this))
        return 0;
    auto obj = GET_EQ(this, WEAR_BACKPACK);
    if (!obj)
        return 0;
    switch (GET_OBJ_VNUM(obj)) {
        case 1806:
            return 1;
        case 1807:
            return 2;
        default:
            return 0;
    }
}

int64_t char_data::calcGravCost(int64_t num) {
    double gravity = 1.0;
    auto room = world.find(in_room);
    if (room != world.end()) {
        gravity = room->second.getGravity();
    }
    int64_t cost = (gravity * gravity);

    if (!num) {
        if (cost) {
            send_to_char(this, "You sweat bullets struggling against a mighty burden.\r\n");
        }
        if ((this->getCurST()) > cost) {
            this->decCurST(cost);
            return 1;
        } else {
            this->decCurST(cost);
            return 0;
        }
    } else {
        return (this->getCurST()) > (cost + num);
    }
}


int64_t char_data::getCurHealth() {
    return getCurPL();
}

int64_t char_data::getMaxHealth() {
    return getMaxPL();
}

double char_data::getCurHealthPercent() {
    return getCurPLPercent();
}

int64_t char_data::getPercentOfCurHealth(double amt) {
    return getPercentOfCurPL(amt);
}

int64_t char_data::getPercentOfMaxHealth(double amt) {
    return getPercentOfMaxPL(amt);
}

bool char_data::isFullHealth() {
    return isFullPL();
}

int64_t char_data::setCurHealth(int64_t amt) {
    return 0;
}

int64_t char_data::setCurHealthPercent(double amt) {
    return 0;
}

int64_t char_data::incCurHealth(int64_t amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + (double) std::abs(amt) / (double) getEffMaxPL());
    else
        health += (double) std::abs(amt) / (double) getEffMaxPL();
    return getCurHealth();
};

int64_t char_data::decCurHealth(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    health = std::max(fl, health - (double) std::abs(amt) / (double) getEffMaxPL());
    return getCurHealth();
}

int64_t char_data::incCurHealthPercent(double amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + std::abs(amt));
    else
        health += std::abs(amt);
    return getCurHealth();
}

int64_t char_data::decCurHealthPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    health = std::max(fl, health - std::abs(amt));
    return getCurHealth();
}

void char_data::restoreHealth(bool announce) {
    if (!isFullHealth()) health = 1;
}

int64_t char_data::getMaxPLTrans() {
    auto form = race->getCurForm(this);
    int64_t total = 0;
    if (form.flag) {
        total = (form.bonus + getEffBasePL()) * form.mult;
    } else {
        total = getEffBasePL() * form.mult;
    }
    return total;
}

int64_t char_data::getMaxPL() {
    auto total = getMaxPLTrans();
    if (GET_KAIOKEN(this) > 0) {
        total += (total / 10) * GET_KAIOKEN(this);
    }
    if (AFF_FLAGGED(this, AFF_METAMORPH)) {
        total += (total * .6);
    }
    return total;
}

int64_t char_data::getCurPL() {
    if (suppression > 0) {
        return getEffMaxPL() * std::min(health, (double) suppression / 100);
    } else {
        return getEffMaxPL() * health;
    }
}

int64_t char_data::getEffBasePL() {
    if (original) return original->getEffBasePL();
    if (!clones.empty()) {
        return getBasePL() / (clones.size() + 1);
    } else {
        return getBasePL();
    }
}

int64_t char_data::getBasePL() {
    return get(CharStat::PowerLevel);
}

double char_data::getCurPLPercent() {
    return (double) getCurPL() / (double) getMaxPL();
}

int64_t char_data::getPercentOfCurPL(double amt) {
    return getCurPL() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxPL(double amt) {
    return getMaxPL() * std::abs(amt);
}

bool char_data::isFullPL() {
    return health >= 1.0;
}

int64_t char_data::getCurKI() {
    return getMaxKI() * energy;
}

int64_t char_data::getMaxKI() {
    auto form = race->getCurForm(this);
    if (form.flag) {
        return (form.bonus + getEffBaseKI()) * form.mult;
    } else {
        return getEffBaseKI();
    }
}

int64_t char_data::getEffBaseKI() {
    if (original) return original->getEffBaseKI();
    if (!clones.empty()) {
        return getBaseKI() / (clones.size() + 1);
    } else {
        return getBaseKI();
    }
}

int64_t char_data::getBaseKI() {
    return get(CharStat::Ki);
}

double char_data::getCurKIPercent() {
    return (double) getCurKI() / (double) getMaxKI();
}

int64_t char_data::getPercentOfCurKI(double amt) {
    return getCurKI() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxKI(double amt) {
    return getMaxKI() * std::abs(amt);
}

bool char_data::isFullKI() {
    return energy >= 1.0;
}

int64_t char_data::setCurKI(int64_t amt) {
    return 0;
}

int64_t char_data::setCurKIPercent(double amt) {
    return 0;
}

int64_t char_data::incCurKI(int64_t amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + (double) std::abs(amt) / (double) getMaxKI());
    else
        energy += (double) std::abs(amt) / (double) getMaxKI();
    return getCurKI();
};

int64_t char_data::decCurKI(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - (double) std::abs(amt) / (double) getMaxKI());
    return getCurKI();
}

int64_t char_data::incCurKIPercent(double amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + std::abs(amt));
    else
        energy += std::abs(amt);
    return getCurKI();
}

int64_t char_data::decCurKIPercent(double amt, int64_t floor) {
    if (!strcasecmp(this->name, "Wayland")) {
        send_to_char(this, "decCurKIPercent called with: %f\r\n", amt);
    }
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - std::abs(amt));
    return getCurKI();
}


void char_data::restoreKI(bool announce) {
    if (!isFullKI()) energy = 1;
}

int64_t char_data::getCurST() {
    return getMaxST() * stamina;
}

int64_t char_data::getMaxST() {
    auto form = race->getCurForm(this);
    if (form.flag) {
        return (form.bonus + getEffBaseST()) * form.mult;
    } else {
        return getEffBaseST();
    }
}

int64_t char_data::getEffBaseST() {
    if (original) return original->getEffBaseST();
    if (!clones.empty()) {
        return getBaseST() / (clones.size() + 1);
    } else {
        return getBaseST();
    }
}

int64_t char_data::getBaseST() {
    return get(CharStat::Stamina);
}

double char_data::getCurSTPercent() {
    return (double) getCurST() / (double) getMaxST();
}

int64_t char_data::getPercentOfCurST(double amt) {
    return getCurST() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxST(double amt) {
    return getMaxST() * std::abs(amt);
}

bool char_data::isFullST() {
    return stamina >= 1;
}

int64_t char_data::setCurST(int64_t amt) {
    return 0;
}

int64_t char_data::setCurSTPercent(double amt) {
    return 0;
}

int64_t char_data::incCurST(int64_t amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxST());
    else
        stamina += (double) std::abs(amt) / (double) getMaxST();
    return getCurST();
};

int64_t char_data::decCurST(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - (double) std::abs(amt) / (double) getMaxST());
    return getCurST();
}

int64_t char_data::incCurSTPercent(double amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + std::abs(amt));
    else
        stamina += std::abs(amt);
    return getMaxST();
}

int64_t char_data::decCurSTPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - std::abs(amt));
    return getCurST();
}


void char_data::restoreST(bool announce) {
    if (!isFullST()) stamina = 1;
}


int64_t char_data::getCurLF() {
    return getMaxLF() * life;
}

int64_t char_data::getMaxLF() {
    auto lb = GET_LIFEBONUSES(this);

    return (IS_DEMON(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.75) + lb
                           : (IS_KONATSU(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.85) +
                    lb : (GET_MAX_MANA(this) * 0.5) +
                                                                         (GET_MAX_MOVE(this) * 0.5) +
                    lb));
}

double char_data::getCurLFPercent() {
    return life;
}

int64_t char_data::getPercentOfCurLF(double amt) {
    return getCurLF() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxLF(double amt) {
    return getMaxLF() * std::abs(amt);
}

bool char_data::isFullLF() {
    return life >= 1.0;
}

int64_t char_data::setCurLF(int64_t amt) {
    life = std::max<int64_t>(0L, std::abs(amt));
    return getCurLF();
}

int64_t char_data::setCurLFPercent(double amt) {
    life = std::max<int64_t>(0L, (int64_t) (getMaxLF() * std::abs(amt)));
    return getCurLF();
}

int64_t char_data::incCurLF(int64_t amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxLF());
    else
        life += (double) std::abs(amt) / (double) getMaxLF();
    return getCurLF();
};

int64_t char_data::decCurLF(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - (double) std::abs(amt) / (double) getMaxLF());
    return getCurLF();
}

int64_t char_data::incCurLFPercent(double amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, life + std::abs(amt));
    else
        life += std::abs(amt);
    return getCurLF();
}

int64_t char_data::decCurLFPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - std::abs(amt));
    return getCurLF();
}


void char_data::restoreLF(bool announce) {
    if (!isFullLF()) life = 1;
}


bool char_data::isFullVitals() {
    return isFullHealth() && isFullKI() && isFullST();
}

void char_data::restoreVitals(bool announce) {
    restoreHealth(announce);
    restoreKI(announce);
    restoreST(announce);
}

void char_data::restoreStatus(bool announce) {
    cureStatusKnockedOut(announce);
    cureStatusBurn(announce);
    cureStatusPoison(announce);
}

void char_data::setStatusKnockedOut() {
    affected_by.set(AFF_KNOCKED);
    affected_by.reset(AFF_FLYING);
    altitude = 0;
    GET_POS(this) = POS_SLEEPING;
}

void char_data::cureStatusKnockedOut(bool announce) {
    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        if (announce) {
            ::act("@W$n@W is no longer senseless, and wakes up.@n", false, this, nullptr, nullptr, TO_ROOM);
            send_to_char(this, "You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(this)) {
            if (GET_ALIGNMENT(CARRIED_BY(this)) > 50) {
                carry_drop(CARRIED_BY(this), 0);
            } else {
                carry_drop(CARRIED_BY(this), 1);
            }
        }

        affected_by.reset(AFF_KNOCKED);
        GET_POS(this) = POS_SITTING;
    }
}

void char_data::cureStatusBurn(bool announce) {
    if (AFF_FLAGGED(this, AFF_BURNED)) {
        if (announce) {
            send_to_char(this, "Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        affected_by.reset(AFF_BURNED);
    }
}

void char_data::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static std::map<int, std::string> limb_names = {
        {0, "right arm"},
        {1, "left arm"},
        {2, "right leg"},
        {3, "left leg"}
};

void char_data::restoreLimbs(bool announce) {
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for (const auto &l: limb_names) {
        if (announce) {
            if (GET_LIMBCOND(this, l.first) <= 0)
                send_to_char(this, "Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                send_to_char(this, "Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->race->gainTail(this, announce);
}

int64_t char_data::gainBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, amt);
}

int64_t char_data::gainBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, amt);
}

int64_t char_data::gainBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, amt);
}

void char_data::gainBaseAll(int64_t amt, bool trans_mult) {
    gainBasePL(amt, trans_mult);
    gainBaseKI(amt, trans_mult);
    gainBaseST(amt, trans_mult);
}

int64_t char_data::loseBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, -amt);
}

int64_t char_data::loseBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, -amt);
}

int64_t char_data::loseBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, -amt);
}

void char_data::loseBaseAll(int64_t amt, bool trans_mult) {
    loseBasePL(amt, trans_mult);
    loseBaseKI(amt, trans_mult);
    loseBaseST(amt, trans_mult);
}

int64_t char_data::gainBasePLPercent(double amt, bool trans_mult) {
    return gainBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t char_data::gainBaseKIPercent(double amt, bool trans_mult) {
    return gainBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t char_data::gainBaseSTPercent(double amt, bool trans_mult) {
    return gainBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

int64_t char_data::loseBasePLPercent(double amt, bool trans_mult) {
    return loseBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t char_data::loseBaseKIPercent(double amt, bool trans_mult) {
    return loseBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t char_data::loseBaseSTPercent(double amt, bool trans_mult) {
    return loseBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

void char_data::gainBaseAllPercent(double amt, bool trans_mult) {
    gainBasePLPercent(amt, trans_mult);
    gainBaseKIPercent(amt, trans_mult);
    gainBaseSTPercent(amt, trans_mult);
}

void char_data::loseBaseAllPercent(double amt, bool trans_mult) {
    loseBasePLPercent(amt, trans_mult);
    loseBaseKIPercent(amt, trans_mult);
    loseBaseSTPercent(amt, trans_mult);
}


double char_data::getMaxCarryWeight() {
    return (getWeight() + 100.0) + (getMaxPL() / 200.0) + (GET_STR(this) * 50) + (IS_BARDOCK(this) ? 10000.0 : 0.0);
}

double char_data::getEquippedWeight() {
    double total_weight = 0;

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(this, i)) {
            total_weight += GET_OBJ_WEIGHT(GET_EQ(this, i));
        }
    }
    return total_weight;
}

double char_data::getCarriedWeight() {
    return getEquippedWeight() + getInventoryWeight() + (carrying ? carrying->getTotalWeight() : 0);
}

double char_data::getAvailableCarryWeight() {
    return getMaxCarryWeight() - getCarriedWeight();
}

double char_data::speednar() {
    auto ratio = (double) getCarriedWeight() / (double) getMaxCarryWeight();
    if (ratio >= .05)
        return std::max(0.01, std::min(1.0, 1.0 - ratio));
    return 1.0;
}

int64_t char_data::getEffMaxPL() {
    if (IS_NPC(this)) {
        return getMaxPL();
    }
    return getMaxPL() * speednar();
}

bool char_data::isWeightedPL() {
    return getMaxPL() > getEffMaxPL();
}

void char_data::apply_kaioken(int times, bool announce) {
    GET_KAIOKEN(this) = times;
    playerFlags.reset(PLR_POWERUP);

    if (announce) {
        send_to_char(this, "@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr,
              nullptr, TO_ROOM);
    }

}

void char_data::remove_kaioken(int8_t announce) {
    auto kaio = GET_KAIOKEN(this);
    if (!kaio) {
        return;
    }
    GET_KAIOKEN(this) = 0;

    switch (announce) {
        case 1:
            send_to_char(this, "You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            send_to_char(this, "You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}

std::optional<vnum> char_data::getMatchingArea(std::function<bool(const area_data &)> f) {
    if(auto room = getRoom(); room) {
        return room->getMatchingArea(f);
    }
    return std::nullopt;
}

int char_data::getRPP() {
    if(IS_NPC(this)) {
        return 0;
    }

    auto &p = players[id];

    return p.account->rpp;

}

void account_data::modRPP(int amt) {
    rpp += amt;
    if(rpp < 0) {
        rpp = 0;
    }
    dirty_accounts.insert(vn);
}

void char_data::modRPP(int amt) {
    if(IS_NPC(this)) {
        return;
    }

    auto &p = players[id];

    p.account->modRPP(amt);
}

int char_data::getPractices() {
    return practice_points;
}

void char_data::modPractices(int amt) {
    practice_points += amt;
    if(practice_points < 0) {
        practice_points = 0;
    }
}


void char_data::login() {
    enter_player_game(desc);
    send_to_char(this, "%s", CONFIG_WELC_MESSG);
    ::act("$n has entered the game.", true, this, nullptr, nullptr, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(this)), true, "%s has entered the game.", GET_NAME(this));
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
    auto count = 0;
    auto oldcount = HIGHPCOUNT;
    struct descriptor_data *k;

    for (k = descriptor_list; k; k = k->next) {
        if (!IS_NPC(k->character) && GET_LEVEL(k->character) > 3) {
            count += 1;
        }

        if (count > PCOUNT) {
            PCOUNT = count;
        }

        if (PCOUNT >= HIGHPCOUNT) {
            oldcount = HIGHPCOUNT;
            HIGHPCOUNT = PCOUNT;
            PCOUNTDATE = ::time(nullptr);
        }

    }

    time.logon = ::time(nullptr);
    greet_mtrigger(this, -1);
    greet_memory_mtrigger(this);

    STATE(desc) = CON_PLAYING;
    if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
        payout(0);
    }
    if (PCOUNT == HIGHPCOUNT) {
        payout(1);
    }
    if (PCOUNT > oldcount) {
        payout(2);
    }

    /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
    if (GET_LEVEL(this) == 0) {
        do_start(this);
        send_to_char(this, "%s", CONFIG_START_MESSG);
    }
    if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1 && GET_LOADROOM(this) != NOWHERE) {
        char_from_room(this);
        char_to_room(this, real_room(real_room(GET_LOADROOM(this))));
    } else if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1) {
        char_from_room(this);
        char_to_room(this, real_room(real_room(300)));
    } else {
        look_at_room(IN_ROOM(this), this, 0);
    }
    if (has_mail(GET_IDNUM(this)))
        send_to_char(this, "\r\nYou have mail waiting.\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWIMM > GET_BOARD(this, 1))
        send_to_char(this,
                     "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWCOD > GET_BOARD(this, 2))
        send_to_char(this,
                     "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWBUI > GET_BOARD(this, 4))
        send_to_char(this,
                     "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWDUO > GET_BOARD(this, 3))
        send_to_char(this,
                     "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(this, 0))
        send_to_char(this, "\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(this))
        send_to_char(this,
                     "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n",
                     LASTNEWS);

    if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(this)) {
        int diff = (LASTINTEREST - GET_LINTEREST(this));
        int mult = 0;
        while (diff > 0) {
            if ((diff - 86400) < 0 && mult == 0) {
                mult = 1;
            } else if ((diff - 86400) >= 0) {
                diff -= 86400;
                mult++;
            } else {
                diff = 0;
            }
        }
        if (mult > 3) {
            mult = 3;
        }
        GET_LINTEREST(this) = LASTINTEREST;
        if (GET_BANK_GOLD(this) > 0) {
            int inc = ((GET_BANK_GOLD(this) / 100) * 2);
            if (inc >= 7500) {
                inc = 7500;
            }
            inc *= mult;
            set(CharMoney::Bank, inc);
            send_to_char(this, "Interest happened while you were away, %d times.\r\n"
                                       "@cBank Interest@D: @Y%s@n\r\n", mult, add_commas(inc).c_str());
        }
    }

    if (!IS_ANDROID(this)) {
        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense someone appear suddenly", this);
        sprintf(buf3,
                "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                add_commas(GET_HIT(this)).c_str());
        send_to_scouter(buf3, this, 0, 0);
    }

    desc->has_prompt = 0;
    /* We've updated to 3.1 - some bits might be set wrongly: */
    pref.reset(PRF_BUILDWALK);
    if (!GET_EQ(this, WEAR_WIELD1) && PLR_FLAGGED(this, PLR_THANDW)) {
        playerFlags.reset(PLR_THANDW);
    }

}

int char_data::getAffectModifier(int location, int specific) {
    int total = 0;
    for(auto a = affected; a; a = a->next) {
        if(location != a->location) continue;
        if(specific != -1 && specific != a->specific) continue;
        total += a->modifier;
    }
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
        total += obj->getAffectModifier(location, specific);
    }
    return total;
}

align_t char_data::get(CharAlign type) {
    if(auto find = aligns.find(type); find != aligns.end()) {
        return find->second;
    }
    return 0;
}

align_t char_data::set(CharAlign type, align_t val) {
    return aligns[type] = std::clamp<align_t>(val, -1000, 1000);
}

align_t char_data::mod(CharAlign type, align_t val) {
    return set(type, get(type) + val);
}

appearance_t char_data::get(CharAppearance type) {
    if(auto find = appearances.find(type); find != appearances.end()) {
        return find->second;
    }
    return 0;
}

appearance_t char_data::set(CharAppearance type, appearance_t val) {
    return appearances[type] = std::clamp<appearance_t>(val, 0, 100);
}

appearance_t char_data::mod(CharAppearance type, appearance_t val) {
    return set(type, get(type) + val);
}

int char_data::setSize(int val) {
    this->size = val;
    return this->size;
}

int char_data::getSize() {
    return get_size(this);
}


money_t char_data::get(CharMoney mon) {
    if(auto find = moneys.find(mon); find != moneys.end()) {
        return find->second;
    }
    return 0;
}

money_t char_data::set(CharMoney mon, money_t val) {
    return moneys[mon] = std::min<money_t>(val, 999999999999);
}

money_t char_data::mod(CharMoney mon, money_t val) {
    return set(mon, get(mon) + val);
}


attribute_t char_data::get(CharAttribute attr, bool base) {
    attribute_t val = 0;
    if(auto stat = attributes.find(attr); stat != attributes.end()) {
        val = stat->second;
    }
    if(!base) {
        val += getAffectModifier((int)attr+1) + getAffectModifier(APPLY_ALL_STATS);
        return std::clamp<attribute_t>(val, 5, 100);
    }
    return val;

}

attribute_t char_data::set(CharAttribute attr, attribute_t val) {
    return attributes[attr] = std::clamp<attribute_t>(val, 0, 80);
}

attribute_t char_data::mod(CharAttribute attr, attribute_t val) {
    return set(attr, get(attr) + val);
}

attribute_train_t char_data::get(CharTrain attr) {
    if(auto t = trains.find(attr); t != trains.end()) {
        return t->second;
    }
    return 0;
}

attribute_train_t char_data::set(CharTrain attr, attribute_train_t val) {
    return trains[attr] = std::max<attribute_train_t>(0, val);
}

attribute_train_t char_data::mod(CharTrain attr, attribute_train_t val) {
    return set(attr, get(attr) + val);
}


num_t char_data::get(CharNum stat) {
    if(auto st = nums.find(stat); st != nums.end()) {
        return st->second;
    }
    return 0;
}

num_t char_data::set(CharNum stat, num_t val) {
    return nums[stat] = val;
}

num_t char_data::mod(CharNum stat, num_t val) {
    return set(stat, get(stat) + val);
}

stat_t char_data::set(CharStat type, stat_t val) {
    return stats[type] = std::max<stat_t>(0, val);
}

stat_t char_data::mod(CharStat type, stat_t val) {
    return set(type, get(type) + val);
}

stat_t char_data::get(CharStat type) {
    if(auto st = stats.find(type); st != stats.end()) {
        return st->second;
    }
    return 0;
}


bool char_data::canCarryWeight(weight_t val) {
    double gravity = 1.0;
    auto room = world.find(in_room);
    if(room != world.end()) {
        gravity = room->second.getGravity();
    }
    return getAvailableCarryWeight() >= (val * gravity);
}

bool char_data::canCarryWeight(struct obj_data *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

bool char_data::canCarryWeight(struct char_data *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

weight_t char_data::getCurrentBurden() {
    auto total = getTotalWeight();
    auto room = world.find(in_room);
    if(room != world.end()) {
        total *= room->second.getGravity();
    }
    return total;
}

double char_data::getBurdenRatio() {
    auto total = getCurrentBurden();
    auto max = getMaxCarryWeight();
    if(max == 0) return 0;
    return total / max;
}

room_vnum char_data::normalizeLoadRoom(room_vnum in) {
    // If they were in the void, then we need to use their last good room.
    room_vnum room = NOWHERE;
    room_vnum lroom = NOWHERE;
    // Handle the void issue...
    if (in == 0 || in == 1) {
        room = GET_WAS_IN(this);
    } else {
        room = in;
    }

    // Personal Pocket Dimensions
    //if (room >= 19800 && room <= 19899) {
        //lroom = room;
    //}
        // those stuck in the pendulum room past get returned to Kami's Lookout.
    if (ROOM_FLAGGED(room, ROOM_PAST)) {
        lroom = 1561;
    }
        // the WMAT arena also is not a good place to log off.
    else if (room >= 2002 && room <= 2011) {
        lroom = 1960;
    }
        // The two Minecarts are possible trap zones.
    else if (room == 2069) {
        lroom = 2017;
    }
    else if (room == 2070) {
        lroom = 2046;
    }
        // The higher plane is a problem...
    else if(room == 6030) {
        // Stick them on the side of King Yemma's, in case they're broke/weak.
        lroom = 6029;
    }
        // This is the MUD School. If they're not done then put them
        // back at the start. Otherwise, send them to their Sensei.
    else if (room >= 101 && room <= 139) {
        if (GET_LEVEL(this) == 1) {
            lroom = 100;
            GET_EXP(this) = 0;
        } else {
            lroom = chclass->senseiStartRoom();
        }
    }
    else {
        // looks like room might be okay.
        lroom = room;
    }

    // if lroom is valid, save it... else... emergency fallback to mud school.
    if(world.contains(lroom)) return lroom;
    return CONFIG_MORTAL_START;

}

std::map<int, obj_data *> char_data::getEquipment() {
    std::map<int, obj_data*> out;
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto eq = GET_EQ(this, i); eq) {
            out[i] = eq;
        }
    }
    return out;
}

obj_data* char_data::getEquipSlot(int slot) {
    if(slot < 0 || slot > NUM_WEARS-1) return nullptr;
    return GET_EQ(this, slot);
}


int char_data::getArmor() {
    int out = get(CharNum::ArmorWishes) * 5000;
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
            out += obj->getAffectModifier(APPLY_AC, -1);
    }
    return out;
}