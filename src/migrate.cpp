#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"
#include "dbat/genolc.h"
#include "dbat/maputils.h"
#include "dbat/config.h"
#include <filesystem>
#include <memory>
#include <iostream>
#include <vector>
#include <tuple>
#include "SQLiteCpp/SQLiteCpp.h"
#include <fstream>
#include "dbat/class.h"
#include "dbat/players.h"
#include "nlohmann/json.hpp"
#include "dbat/account.h"
#include "dbat/objsave.h"
#include "dbat/house.h"

static std::string stripAnsi(const std::string& str) {
    return processColors(str, false, nullptr);
}

struct old_ship_data {
    std::string name;
    std::set<vnum> vnums;
    std::optional<vnum> hatch_room{};
    std::optional<vnum> ship_obj{};
    std::optional<vnum> location{};
};

static struct old_ship_data gships[] = {
        {"Falcon", {3900, 3901, 3902, 3903, 3904}, 3900, 3900, 408},
        {"Simurgh", {3905, 3996, 3997, 3998, 3999}, 3905, 3905, 12002},
        {"Zypher", {3906, 3907, 3908, 3909, 3910}, 3906, 3906, 4250},
        {"Valkyrie", {3911, 3912, 3913, 3914, 3915}, 3911, 3911, 2323},
        {"Phoenix", {3916, 3917, 3918, 3919, 3920}, 3916, 3916, 408},
        {"Merganser", {3921, 3922, 3923, 3924, 3925}, 3921, 3921, 2323},
        {"Wraith", {3926, 3927, 3928, 3929, 3930}, 3930, 3930, 11626},
        {"Ghost", {3931, 3932, 3933, 3934, 3935}, 3935, 3935, 8194},
        {"Wisp", {3936, 3937, 3938, 3939, 3940}, 3940, 3940, 12002},
        {"Eagle", {3941, 3942, 3943, 3944, 3945}, 3945, 3945, 4250},

        {"Spectral", {3946, 3947, 3948, 3949, 3950}, 3950, {}, {}},
        {"Raven", {3951, 3952, 3953, 3954, 3955, 3961}, 3955, {}, {}},
        {"Marauder", {3956, 3957, 3958, 3959, 3960}, 3960, {}, {}},
        {"Vanir", {3962, 3963, 3964, 3965}, 3965, {}, {}},
        {"Aesir", {3966, 3967, 3968, 3969, 3970}, 3970, {}, {}},
        {"Undine", {3971, 3972, 3973, 3974, 3975}, 3975, {}, {}},
        {"Banshee", {3976, 3977, 3978, 3979, 3980}, 3980, {}, {}},
        {"Hydra", {3981, 3982, 3983, 3984, 3985}, 3985, {}, {}},
        {"Medusa", {3986, 3987, 3988, 3989, 3990}, 3990, {}, {}},
        {"Pegasus", {3991, 3992, 3993, 3994, 3995}, 3995, {}, {}},
        {"Zel 1", {5824}, 5824, {}, {}},
        {"Zel 2", {5825}, 5825, {}, {}}
};

static struct old_ship_data customs[] = {
        {"Yun-Yammka", {19900, 19901, 19902}, 19901, 19900, {}},
        {"The Dark Archon", {19903, 19912, 19913, 19914}, 19912, 19916, {}},
        {"The Zafir Krakken", {19904, 19905, 19906, 19907}, 19905, 19905, {}},
        {"Crimson Talon", {19908, 19909, 19910, 19911}, 19908, 19910, {}},
        {"Rust Bucket", {19915, 19916, 19918, 19930}, 19915, 19921, {}},
        {"The Adamant", {19917, 19949, 19955, 19956}, 19949, 19966, {}},
        {"Vanguard", {19919, 19920, 19921, 19922}, 19920, 19926, {}},
        {"The Glacial Shadow", {19925, 19923, 19924, 19926}, 19923, 19931, {}},
        {"The Molecular Dynasty", {19927, 19928, 19929, 19954}, 19927, 19936, {}},
        {"Poseidon", {19931, 19933, 19932, 19934}, 19931, 19941, {}},
        {"Sakana Mirai", {19935, 19936, 19937, 19938}, 19935, 19946, {}},
        {"Earth Far Freighter Enterjection", {19939, 19940, 19941, 19942}, 19939, 19951, {}},
        {"Soaring Angel", {19943, 19944, 19945, 19946}, 19943, 19956, {}},
        {"The Grey Wolf", {19947, 19948, 19978, 19979}, 19947, 19961, {}},
        {"The Event Horizon", {19950, 19951, 19980, 19981}, 19950, 19971, {}},
        {"Fleeting Star", {19952, 19953, 19957, 19958}, 19952, 19976, {}},
        {"Makenkosappo", {19959, 19960, 19961, 19962}, 19959, 19981, {}},
        {"The Nightingale", {19963, 19964, 19965, 19982}, 19963, 19986, {}},
        {"The Honey Bee", {19966, 19967, 19968, 19969}, 19966, 19991, {}},
        {"The Bloodrose", {19970, 19971, 19972, 19973}, 19970, 19996, {}},
        {"The Stimato", {19974, 19975, 19976, 19977}, 19974, {}, {}},
        {"The Tatsumaki", {15805, 15806, 15807, 15808}, 15805, 15805, {}},
        {"Shattered Soul", {15800, 15801, 15802, 15803}, 15800, {}, {}}
};

struct AreaDef {
    std::string name;
    AreaType type{AreaType::Region};
    std::optional<vnum> parent, orbit;
    std::optional<double> gravity;
    std::set<std::size_t> roomFlags{};
    std::vector<std::pair<std::size_t, std::size_t>> roomRanges;
    std::set<vnum> roomIDs{}, roomSkips{};
    std::bitset<NUM_AREA_FLAGS> flags;
};

vnum assembleArea(const AreaDef &def) {
    auto vn = area_data::getNextID();
    auto &a = areas[vn];
    a.vn = vn;
    a.name = def.name;
    a.type = def.type;
    a.flags = def.flags;

    if(def.gravity) {
        a.gravity = def.gravity.value();
    }

    if(def.parent) {
        auto &p = areas[def.parent.value()];
        p.children.insert(vn);
        a.parent = p.vn;
    }

    std::set<vnum> rooms = def.roomIDs;
    a.extraVn = def.orbit;
    if(a.type == AreaType::CelestialBody && a.extraVn) {
        rooms.insert(a.extraVn.value());
    }

    for(auto &[start, end] : def.roomRanges) {
        for(auto i = start; i <= end; i++) {
            auto found = world.find(i);
            if(found == world.end()) continue;
            rooms.insert(i);
        }
    }

    if(!def.roomFlags.empty()) {
        for(auto &[vn, room] : world) {
            for(auto &f : def.roomFlags) {
                if(room.room_flags.test(f)) {
                    rooms.insert(vn);
                    break;
                }
            }
        }
    }

    for(auto r: def.roomSkips) rooms.erase(r);

    basic_mud_log("Assembling Area: %s, Rooms: %d", def.name.c_str(), rooms.size());

    for(auto r : rooms) {
        auto found = world.find(r);
        if(found == world.end()) continue;
        auto &room = found->second;
        if(room.area) continue;
        room.area = vn;
        a.rooms.insert(r);
    }

    return vn;

}


void migrate_grid() {
    AreaDef adef;
    adef.name = "Admin Land";
    adef.roomRanges.emplace_back(0, 16);
    adef.roomIDs = {16694, 16698};
    adef.type = AreaType::Dimension;
    auto admin_land = assembleArea(adef);

    AreaDef mudschooldef;
    mudschooldef.name = "MUD School";
    mudschooldef.roomRanges.emplace_back(100, 154);
    mudschooldef.type = AreaType::Dimension;
    auto mud_school = assembleArea(mudschooldef);

    AreaDef mvdef;
    mvdef.name = "Multiverse";
    mvdef.type = AreaType::Dimension;
    auto multiverse = assembleArea(mvdef);

    AreaDef xvdef;
    xvdef.name = "Xenoverse";
    xvdef.type = AreaType::Dimension;
    auto xenoverse = assembleArea(xvdef);

    AreaDef u7def;
    u7def.name = "Universe 7";
    u7def.type = AreaType::Dimension;
    u7def.parent = multiverse;
    auto universe7 = assembleArea(u7def);

    AreaDef mplane;
    mplane.name = "Mortal Plane";
    mplane.type = AreaType::Dimension;
    mplane.parent = universe7;
    auto mortal_plane = assembleArea(mplane);

    AreaDef cplane;
    cplane.name = "Celestial Plane";
    cplane.type = AreaType::Dimension;
    cplane.parent = universe7;
    auto celestial_plane = assembleArea(cplane);
    
    AreaDef spacedef;
    spacedef.name = "Depths of Space";
    spacedef.type = AreaType::Region;
    spacedef.parent = mortal_plane;
    // Insert every room id from mapnums (the 2d array) into spacedef.roomIDs...
    for(auto &row : mapnums) {
        for(auto &col : row) {
            spacedef.roomIDs.insert(col);
        }
    }
    auto space = assembleArea(spacedef);

    std::unordered_map<std::string, AreaDef> areaDefs;

    { // Earth miscellaneous rooms...
        auto &w = areaDefs["West City"];
        w.roomRanges.emplace_back(19500, 19558);
        w.roomIDs.insert(19576);
        w.roomIDs.insert(19599);
        w.roomIDs.insert(178);
        auto &s = areaDefs["Silver Mine"];
        s.roomIDs.insert(19577);
        auto &n = areaDefs["Nexus City"];
        n.roomIDs.insert(5827);
        n.roomIDs.insert(199);
        n.roomIDs.insert(19);
        n.roomIDs.insert(20);
        n.roomIDs.insert(25);
        n.roomIDs.insert(29);
        n.roomIDs.insert(81);
        n.roomIDs.insert(97);
        n.roomIDs.insert(98);
        n.roomIDs.insert(99);
        n.roomIDs.insert(19001);
        n.roomIDs.insert(19007);
        n.roomIDs.insert(23);
    }

    {// Vegeta misc..
        auto &v = areaDefs["Vegetos City"];
        v.roomIDs.insert(15700);
        v.roomIDs.insert(82);
        v.roomIDs.insert(19003);
        v.roomIDs.insert(179);
        auto &b = areaDefs["Blood Dunes"];
        b.roomIDs.insert(155);
        b.roomIDs.insert(156);
    }

    {
        // Frigid misc...
        auto &i = areaDefs["Ice Crown City"];
        i.roomIDs.insert(83);
        i.roomIDs.insert(19002);
        i.roomIDs.insert(180);
    }

    {
        // Aether misc...
        auto &h = areaDefs["Haven City"];
        h.roomIDs.insert(85);
        h.roomIDs.insert(183);
        h.roomIDs.insert(19005);
        h.roomIDs.insert(19008);
    }

    {
        // yardrat...
        auto &y = areaDefs["Yardra City"];
        y.roomIDs.insert(26);
    }
    {
        // Konack
        auto &t = areaDefs["Tiranoc City"];
        t.roomIDs.insert(86);
        t.roomIDs.insert(181);
        t.roomIDs.insert(19004);
    }

    {
        // Namek stuff..
        auto &k = areaDefs["Kakureta Village"];
        k.roomRanges.emplace_back(14400, 14499);
        auto &s = areaDefs["Senzu Village"];
        s.roomIDs.insert(84);
        s.roomIDs.insert(182);
        auto &e = areaDefs["Elder Village"];
        e.roomIDs.insert(19006);
    }

    {
        // kanassa misc...
        auto &a = areaDefs["Aquis City"];
        a.roomIDs.insert(177);
    }

    for(auto &[rv, room] : world) {
        if(room.area) continue;
        auto sense = sense_location_name(rv);
        if(sense != "Unknown.") {
            auto &area = areaDefs[sense];
            area.roomIDs.insert(rv);
        }
    }

    std::unordered_map<std::string, vnum> areaObjects;

    for(auto &[name, def] : areaDefs) {
        def.name = name;
        def.type = AreaType::Region;
        auto aent = assembleArea(def);
        areaObjects[name] = aent;
    }

    AreaDef pearth;
    pearth.name = "@GEarth@n";
    pearth.type = AreaType::CelestialBody;
    pearth.parent = space;
    pearth.orbit = 50;
    auto planet_earth = assembleArea(pearth);

    AreaDef pvegeta;
    pvegeta.name = "@YVegeta@n";
    pvegeta.type = AreaType::CelestialBody;
    pvegeta.parent = space;
    pvegeta.gravity = 10.0;
    pvegeta.orbit = 53;
    auto planet_vegeta = assembleArea(pvegeta);

    AreaDef pfrigid;
    pfrigid.name = "@CFrigid@n";
    pfrigid.type = AreaType::CelestialBody;
    pfrigid.parent = space;
    pfrigid.orbit = 51;
    auto planet_frigid = assembleArea(pfrigid);

    AreaDef pnamek;
    pnamek.name = "@gNamek@n";
    pnamek.type = AreaType::CelestialBody;
    pnamek.parent = space;
    pnamek.orbit = 54;
    auto planet_namek = assembleArea(pnamek);

    AreaDef pkonack;
    pkonack.name = "@MKonack@n";
    pkonack.type = AreaType::CelestialBody;
    pkonack.parent = space;
    pkonack.orbit = 52;
    auto planet_konack = assembleArea(pkonack);

    AreaDef paether;
    paether.name = "@MAether@n";
    paether.type = AreaType::CelestialBody;
    paether.parent = space;
    paether.orbit = 55;
    auto planet_aether = assembleArea(paether);

    AreaDef pyardrat;
    pyardrat.name = "@mYardrat@n";
    pyardrat.type = AreaType::CelestialBody;
    pyardrat.parent = space;
    pyardrat.orbit = 56;
    auto planet_yardrat = assembleArea(pyardrat);

    AreaDef pkanassa;
    pkanassa.name = "@BKanassa@n";
    pkanassa.type = AreaType::CelestialBody;
    pkanassa.parent = space;
    pkanassa.orbit = 58;
    auto planet_kanassa = assembleArea(pkanassa);

    AreaDef pcerria;
    pcerria.name = "@RCerria@n";
    pcerria.type = AreaType::CelestialBody;
    pcerria.parent = space;
    pcerria.orbit = 198;
    auto planet_cerria = assembleArea(pcerria);

    AreaDef parlia;
    parlia.name = "@GArlia@n";
    parlia.type = AreaType::CelestialBody;
    parlia.parent = space;
    parlia.orbit = 59;
    auto planet_arlia = assembleArea(parlia);

    AreaDef pzenith;
    pzenith.name = "@BZenith@n";
    pzenith.type = AreaType::CelestialBody;
    pzenith.parent = space;
    pzenith.orbit = 57;
    auto moon_zenith = assembleArea(pzenith);
    for(const auto& name : {"Ancient Castle", "Utatlan City", "Zenith Jungle"}) {
        auto vn = areaObjects[name];
        auto &a = areas[vn];
        a.parent = moon_zenith;
        auto &m = areas[moon_zenith];
        m.children.insert(vn);
    }


    AreaDef ucdef;
    ucdef.name = "Underground Cavern";
    ucdef.parent = moon_zenith;
    ucdef.roomRanges.emplace_back(62900, 63000);
    auto underground_cavern = assembleArea(ucdef);

    for(auto &p : {planet_earth, planet_aether, planet_namek, moon_zenith}) {
		auto &planet = areas[p];
        planet.flags.set(AREA_ETHER);
    }

    for(auto &p : {planet_earth, planet_aether, planet_vegeta, planet_frigid}) {
        auto &planet = areas[p];
        planet.flags.set(AREA_MOON);
    }

    AreaDef zelakinfarm;
    zelakinfarm.name = "Zelakin's Farm";
    zelakinfarm.parent = xenoverse;
    zelakinfarm.roomRanges.emplace_back(5896, 5899);
    auto zelakin_farm = assembleArea(zelakinfarm);

    AreaDef hbtcdef;
    hbtcdef.name = "Hyperbolic Time Chamber";
    hbtcdef.parent = universe7;
    hbtcdef.roomRanges.emplace_back(64000, 64097);
    hbtcdef.type = AreaType::Dimension;
    auto hbtc = assembleArea(hbtcdef);

    AreaDef bodef;
    bodef.name = "The Black Omen";
    bodef.parent = space;
    bodef.roomIDs.insert(19053);
    bodef.roomIDs.insert(19039);
    for(auto &[r, room] : world) {
        if(icontains(stripAnsi(room.name), "Black Omen")) bodef.roomIDs.insert(r);
    }
    bodef.roomIDs.insert(19050);
    bodef.type = AreaType::Vehicle;
    auto black_omen = assembleArea(bodef);

    AreaDef earthduel;
    earthduel.name = "Duel Dome";
    earthduel.parent = planet_earth;
    earthduel.roomRanges.emplace_back(160, 176);
    auto earth_duel_dome = assembleArea(earthduel);

    AreaDef earthwmat;
    earthwmat.name = "World Martial Arts Building";
    earthwmat.parent = planet_earth;
    earthwmat.roomRanges.emplace_back(3800, 3834);
    earthwmat.roomRanges.emplace_back(19578, 19598);
    earthwmat.roomRanges.emplace_back(19570, 19573);
    earthwmat.roomIDs = {19574, 19575};
    auto earth_wmat = assembleArea(earthwmat);

    AreaDef capsulecorp;
    capsulecorp.name = "Capsule Corporation";
    capsulecorp.parent = areaObjects["West City"];
    capsulecorp.roomRanges.emplace_back(19559, 19569);
    auto capsule_corp = assembleArea(capsulecorp);

    AreaDef threestarelem;
    threestarelem.name = "Three Star Elementary";
    threestarelem.parent = planet_earth;
    threestarelem.roomRanges.emplace_back(5800, 5823);
    threestarelem.roomIDs.insert(5826);
    auto three_star_elem = assembleArea(threestarelem);

    AreaDef gerol;
    gerol.name = "Gero's Lab";
    gerol.parent = planet_earth;
    gerol.roomRanges.emplace_back(7701, 7753);
    auto gero_lab = assembleArea(gerol);

    AreaDef shadowrain;
    shadowrain.name = "Shadowrain City";
    shadowrain.parent = planet_earth;
    shadowrain.roomRanges.emplace_back(9111, 9199);
    auto shadowrain_city = assembleArea(shadowrain);

    AreaDef kingcastle;
    kingcastle.name = "King Castle";
    kingcastle.parent = planet_earth;
    kingcastle.roomRanges.emplace_back(12600, 12627);
    auto king_castle = assembleArea(kingcastle);

    AreaDef orangestar;
    orangestar.name = "Orange Star Highschool";
    orangestar.parent = planet_earth;
    orangestar.roomRanges.emplace_back(16400, 16499);
    auto orange_star = assembleArea(orangestar);

    AreaDef ath;
    ath.name = "Athletic Field";
    ath.parent = orange_star;
    ath.roomRanges.emplace_back(15900, 15937);
    auto athletic_field = assembleArea(ath);

    AreaDef oak;
    oak.name = "Inside an Oak Tree";
    oak.parent = areaObjects["Northern Plains"];
    oak.roomRanges.emplace_back(16200, 16210);
    oak.roomIDs = {19199};
    auto oak_tree = assembleArea(oak);

    AreaDef edfhq;
    edfhq.name = "EDF Headquarters";
    edfhq.parent = planet_earth;
    edfhq.type = AreaType::Structure;
    edfhq.roomRanges.emplace_back(9101, 9110);
    auto edf_hq = assembleArea(edfhq);

    AreaDef bar;
    bar.name = "Bar";
    bar.parent = planet_earth;
    bar.type = AreaType::Structure;
    bar.roomRanges.emplace_back(18100, 18114);
    auto bar_ = assembleArea(bar);

    AreaDef themoon;
    themoon.name = "The Moon";
    themoon.parent = space;
    themoon.type = AreaType::CelestialBody;
    themoon.gravity = 10.0;
    auto moon = assembleArea(themoon);

    AreaDef luncrat;
    luncrat.name = "Lunar Crater";
    luncrat.parent = moon;
    luncrat.roomRanges.emplace_back(63300, 63311);
    auto lunar_crater = assembleArea(luncrat);

    AreaDef cratpass;
    cratpass.name = "Crater Passage";
    cratpass.parent = moon;
    cratpass.roomRanges.emplace_back(63312, 63336);
    auto crater_passage = assembleArea(cratpass);

    AreaDef darkside;
    darkside.name = "Darkside Crater";
    darkside.parent = moon;
    darkside.roomRanges.emplace_back(63337, 63362);
    auto darkside_crater = assembleArea(darkside);

    AreaDef moonstone;
    moonstone.name = "Moonstone Quarry";
    moonstone.parent = moon;
    moonstone.roomRanges.emplace_back(63381, 63392);
    auto moonstone_quarry = assembleArea(moonstone);

    AreaDef intrepidbase;
    intrepidbase.name = "Intrepid Base";
    intrepidbase.parent = moon;
    intrepidbase.roomRanges.emplace_back(63363, 63380);
    intrepidbase.roomRanges.emplace_back(63393, 63457);
    auto intrepid_base = assembleArea(intrepidbase);

    AreaDef fortemple;
    fortemple.name = "Forgotten Temple";
    fortemple.parent = moon;
    fortemple.roomRanges.emplace_back(63458, 63499);
    auto forgotten_temple = assembleArea(fortemple);
    
    for(auto child : areas[moon].children) {
        auto &a = areas[child];
        for(auto r : a.rooms) {
            ROOM_FLAGS(r).reset(ROOM_EARTH);
        }
    }

    AreaDef prideplains;
    prideplains.name = "Pride Plains";
    prideplains.parent = planet_vegeta;
    prideplains.roomRanges.emplace_back(19700, 19711);
    auto pride_plains = assembleArea(prideplains);

    AreaDef pridesomething;
    pridesomething.name = "Pride Something";
    pridesomething.parent = planet_vegeta;
    pridesomething.roomRanges.emplace_back(19740, 19752);
    auto pride_something = assembleArea(pridesomething);

    AreaDef pridejungle;
    pridejungle.name = "Pride Jungle";
    pridejungle.parent = planet_vegeta;
    pridejungle.roomRanges.emplace_back(19712, 19718);
    pridejungle.roomRanges.emplace_back(19753, 19789);
    auto pride_jungle = assembleArea(pridejungle);

    AreaDef pridecave;
    pridecave.name = "Pride Cave";
    pridecave.parent = planet_vegeta;
    pridecave.roomRanges.emplace_back(9400, 9499);
    auto pride_cave = assembleArea(pridecave);

    AreaDef pridedesert;
    pridedesert.name = "Pride Desert";
    pridedesert.parent = planet_vegeta;
    pridedesert.roomRanges.emplace_back(19719, 19739);
    pridedesert.roomIDs.insert(19790);
    auto pride_desert = assembleArea(pridedesert);

    AreaDef rocktail;
    rocktail.name = "Rocktail Camp";
    rocktail.parent = planet_vegeta;
    rocktail.roomRanges.emplace_back(61030, 61044);
    rocktail.roomIDs.insert(19198);
    auto rocktail_camp = assembleArea(rocktail);

    AreaDef lavaarena;
    lavaarena.name = "Lava Arena";
    lavaarena.parent = planet_frigid;
    lavaarena.roomRanges.emplace_back(12900, 12918);
    auto lava_arena = assembleArea(lavaarena);

    AreaDef strangecliff;
    strangecliff.name = "Strange Cliff";
    strangecliff.parent = planet_namek;
    strangecliff.roomRanges.emplace_back(12800, 12813);
    auto strange_cliff = assembleArea(strangecliff);

    AreaDef stonehallway;
    stonehallway.name = "Stone Hallway";
    stonehallway.parent = planet_namek;
    stonehallway.roomRanges.emplace_back(12814, 12831);
    stonehallway.roomSkips.insert(12825);
    auto stone_hallway = assembleArea(stonehallway);

    AreaDef tranquilpalm;
    tranquilpalm.name = "Tranquil Palm Dojo";
    tranquilpalm.parent = planet_namek;
    tranquilpalm.roomRanges.emplace_back(12832, 12868);
    auto tranquil_palm_dojo = assembleArea(tranquilpalm);

    AreaDef namekunder;
    namekunder.name = "Namekian Underground";
    namekunder.parent = planet_namek;
    namekunder.roomRanges.emplace_back(64700, 65009);
    auto namek_underground = assembleArea(namekunder);

    AreaDef advkindojo;
    advkindojo.name = "Advanced Kinetic Dojo";
    advkindojo.parent = planet_aether;
    advkindojo.roomRanges.emplace_back(17743, 17751);
    auto advanced_kinetic_dojo = assembleArea(advkindojo);

    AreaDef lostcity;
    lostcity.name = "Lost City";
    lostcity.parent = planet_kanassa;
    lostcity.roomRanges.emplace_back(7600, 7686);
    auto lost_city = assembleArea(lostcity);

    AreaDef aqtower;
    aqtower.name = "Aquis Tower";
    aqtower.parent = areaObjects["Aquis City"];
    aqtower.roomRanges.emplace_back(12628, 12666);
    auto aquis_tower = assembleArea(aqtower);

    AreaDef moaipalace;
    moaipalace.name = "Moai's Palace";
    moaipalace.parent = planet_arlia;
    moaipalace.roomRanges.emplace_back(12667, 12699);
    auto moai_palace = assembleArea(moaipalace);

    AreaDef darkthorne;
    darkthorne.name = "DarkThorne Compound";
    darkthorne.parent = planet_arlia;
    darkthorne.roomRanges.emplace_back(18150, 18169);
    auto darkthorne_compound = assembleArea(darkthorne);


    std::unordered_map<int, vnum> planetMap = {
            {ROOM_EARTH, planet_earth},
            {ROOM_VEGETA, planet_vegeta},
            {ROOM_FRIGID, planet_frigid},
            {ROOM_NAMEK, planet_namek},
            {ROOM_YARDRAT, planet_yardrat},
            {ROOM_KONACK, planet_konack},
            {ROOM_AETHER, planet_aether},
            {ROOM_KANASSA, planet_kanassa},
            {ROOM_ARLIA, planet_arlia},
            {ROOM_CERRIA, planet_cerria},
    };

    basic_mud_log("Attempting to deduce Areas to Planets...");
    for(auto &[vnum, room] : world) {
        // check for planetMap flags and, if found, bind the area this room belongs to, to the respective planet.

        for(auto &p : planetMap) {
            if(!room.area) continue;
            if(room.room_flags.test(p.first)) {
                auto avn = room.area.value();
                auto &a = areas[avn];
                auto &pl = areas[p.second];
                pl.children.insert(avn);
                a.parent = p.second;
                break;
            }
        }
    }
    basic_mud_log("Done deducing Areas to Planets.");


    AreaDef nodef;
    nodef.name = "Northran";
    nodef.parent = xenoverse;
    nodef.type = AreaType::Dimension;
    nodef.roomRanges.emplace_back(17900, 17999);
    auto northran = assembleArea(nodef);

    AreaDef celdef;
    celdef.name = "Celestial Corp";
    celdef.parent = space;
    celdef.type = AreaType::Structure;
    celdef.roomRanges.emplace_back(16305, 16399);
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Celestial Corp")) celdef.roomIDs.insert(rv);
    }
    auto celestial_corp = assembleArea(celdef);

    AreaDef gneb;
    gneb.name = "Green Nebula Mall";
    gneb.parent = space;
    gneb.type = AreaType::Structure;
    gneb.roomRanges.emplace_back(17200, 17276);
    gneb.roomIDs.insert(184);
    auto green_nebula = assembleArea(gneb);

    AreaDef cooler;
    cooler.name = "Cooler's Ship";
    cooler.parent = space;
    cooler.type = AreaType::Structure;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Cooler's Ship")) {
            cooler.roomIDs.insert(rv);
        }
    }
    auto cooler_ship = assembleArea(cooler);

    AreaDef alph;
    alph.name = "Alpharis";
    alph.type = AreaType::Structure;
    alph.parent = space;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Alpharis")) alph.roomIDs.insert(rv);
    }
    auto alpharis = assembleArea(alph);

    AreaDef dzone;
    dzone.name = "Dead Zone";
    dzone.parent = universe7;
    dzone.type = AreaType::Dimension;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Dead Zone")) dzone.roomIDs.insert(rv);
    }
    auto dead_zone = assembleArea(dzone);

    AreaDef bast;
    bast.name = "Blasted Asteroid";
    bast.parent = space;
    bast.type = AreaType::CelestialBody;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Blasted Asteroid")) bast.roomIDs.insert(rv);
    }
    auto blasted_asteroid = assembleArea(bast);


    AreaDef listres;
    listres.name = "Lister's Restaurant";
    listres.parent = xenoverse;
    listres.type = AreaType::Structure;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Lister's Restaurant")) listres.roomIDs.insert(rv);
    }
    listres.roomIDs = {18640};
    auto listers_restaurant = assembleArea(listres);

    AreaDef scasino;
    scasino.name = "Shooting Star Casino";
    scasino.type = AreaType::Structure;
    scasino.parent = xenoverse;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "Shooting Star Casino")) scasino.roomIDs.insert(rv);
    }
    auto shooting_star_casino = assembleArea(scasino);

    AreaDef outdef;
    outdef.name = "The Outpost";
    outdef.parent = celestial_plane;
	outdef.type = AreaType::Structure;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room.name), "The Outpost")) outdef.roomIDs.insert(rv);
    }
    auto outpost = assembleArea(outdef);

    AreaDef kyem;
    kyem.name = "King Yemma's Domain";
    kyem.parent = celestial_plane;
    kyem.roomRanges.emplace_back(6000, 6030);
    kyem.roomSkips.insert(6017);
    kyem.roomIDs.insert(6295);
    auto king_yemma = assembleArea(kyem);

    AreaDef snway;
    snway.name = "Snake Way";
    snway.parent = celestial_plane;
    snway.roomRanges.emplace_back(6031, 6099);
    snway.roomIDs.insert(6017);
    auto snake_way = assembleArea(snway);

    AreaDef nkai;
    nkai.name = "North Kai's Planet";
    nkai.parent = celestial_plane;
    nkai.gravity = 10.0;
    nkai.type = AreaType::CelestialBody;
    nkai.roomRanges.emplace_back(6100, 6138);
    auto north_kai = assembleArea(nkai);

    AreaDef serp;
    serp.name = "Serpent's Castle";
    serp.parent = snake_way;
    serp.type = AreaType::Structure;
    serp.roomRanges.emplace_back(6139, 6166);
    auto serpents_castle = assembleArea(serp);

    AreaDef gkai;
    gkai.name = "Grand Kai's Planet";
    gkai.parent = celestial_plane;
    gkai.type = AreaType::CelestialBody;
    gkai.roomRanges.emplace_back(6800, 6960);
    auto grand_kai = assembleArea(gkai);

    AreaDef gkaipalace;
    gkaipalace.name = "Grand Kai's Palace";
    gkaipalace.parent = grand_kai;
    gkaipalace.type = AreaType::Structure;
    gkaipalace.roomRanges.emplace_back(6961, 7076);
    auto grand_kais_palace = assembleArea(gkaipalace);

    AreaDef maze;
    maze.name = "Maze of Echoes";
    maze.parent = celestial_plane;
    maze.roomRanges.emplace_back(7100, 7199);
    auto maze_of_echoes = assembleArea(maze);

    AreaDef cat;
    cat.name = "Dark Catacomb";
    cat.parent = maze_of_echoes;
    cat.roomRanges.emplace_back(7200, 7245);
    auto dark_catacomb = assembleArea(cat);

    AreaDef twi;
    twi.name = "Twilight Cavern";
    twi.parent = celestial_plane;
    twi.roomRanges.emplace_back(7300, 7499);
    auto twilight_cavern = assembleArea(twi);

    AreaDef helldef;
    helldef.name = "Hell";
    helldef.parent = celestial_plane;
    helldef.roomRanges.emplace_back(6200, 6298);
    helldef.roomSkips.insert(6295);
    auto hell = assembleArea(helldef);

    AreaDef hellhouse;
    hellhouse.name = "Hell - Old House";
    hellhouse.parent = hell;
    hellhouse.roomRanges.emplace_back(61000, 61007);
    auto hell_old_house = assembleArea(hellhouse);

    AreaDef gyukihouse;
    gyukihouse.name = "Gyuki's House";
    gyukihouse.parent = planet_earth;
    gyukihouse.roomRanges.emplace_back(61015, 61026);
    auto gyukis_house = assembleArea(gyukihouse);

    AreaDef hfields;
    hfields.name = "Hell Fields";
    hfields.parent = hell;
    hfields.roomRanges.emplace_back(6200, 6300);
    auto hell_fields = assembleArea(hfields);

    AreaDef hsands;
    hsands.name = "Sands of Time";
    hsands.parent = hell;
    hsands.roomRanges.emplace_back(6300, 6348);
    auto sands_of_time = assembleArea(hsands);

    AreaDef hchaotic;
    hchaotic.name = "Chaotic Spiral";
    hchaotic.parent = hell;
    hchaotic.roomRanges.emplace_back(6349, 6399);
    auto chaotic_spiral = assembleArea(hchaotic);

    AreaDef hfirecity;
    hfirecity.name = "Hellfire City";
    hfirecity.parent = hell;
    hfirecity.roomRanges.emplace_back(6400, 6529);
    hfirecity.roomIDs = {6568, 6569, 6600, 6699};
    auto hellfire_city = assembleArea(hfirecity);

    AreaDef fbagdojo;
    fbagdojo.name = "Flaming Bag Dojo";
    fbagdojo.type = AreaType::Structure;
    fbagdojo.parent = hellfire_city;
    fbagdojo.roomRanges.emplace_back(6530, 6568);
    auto flaming_bag_dojo = assembleArea(fbagdojo);

    AreaDef etrailgrave;
    etrailgrave.name = "Entrail Graveyard";
    etrailgrave.parent = hellfire_city;
    etrailgrave.roomRanges.emplace_back(6601, 6689);
    auto entrail_graveyard = assembleArea(etrailgrave);

    AreaDef psihnon;
    psihnon.name = "Sihnon";
    psihnon.parent = space;
    psihnon.type = AreaType::CelestialBody;
    psihnon.roomRanges.emplace_back(3600, 3699);
    auto planet_sihnon = assembleArea(psihnon);

    AreaDef majdef;
    majdef.name = "Majinton";
    majdef.parent = planet_sihnon;
    majdef.type = AreaType::Dimension;
    majdef.roomRanges.emplace_back(3700, 3797);
    auto majinton = assembleArea(majdef);

    AreaDef wistower;
    wistower.name = "Wisdom Tower";
    wistower.parent = planet_namek;
    wistower.type = AreaType::Structure;
    wistower.roomRanges.emplace_back(9600, 9666);
    auto wisdom_tower = assembleArea(wistower);

    AreaDef veld;
    veld.name = "Veldryth Mountains";
    veld.parent = planet_konack;
    veld.roomRanges.emplace_back(9300, 9355);
    auto veldryth_mountains = assembleArea(veld);

    AreaDef machia;
    machia.name = "Machiavilla";
    machia.parent = planet_konack;
    machia.type = AreaType::Structure;
    machia.roomRanges.emplace_back(12743, 12798);
    machia.roomRanges.emplace_back(12700, 12761);
    machia.roomIDs.insert(9356);
    auto machiavilla = assembleArea(machia);

    AreaDef laron;
    laron.name = "Laron Forest";
    laron.parent = planet_konack;
    laron.roomRanges.emplace_back(19200, 19299);
    auto laron_forest = assembleArea(laron);

    AreaDef nazr;
    nazr.name = "Nazrin Village";
    nazr.parent = planet_konack;
    nazr.roomRanges.emplace_back(19300, 19347);
    nazr.roomIDs = {19398};
    auto nazrin_village = assembleArea(nazr);

    AreaDef nazchief;
    nazchief.name = "Chieftain's House";
    nazchief.type = AreaType::Structure;
    nazchief.parent = nazrin_village;
    nazchief.roomRanges.emplace_back(19348, 19397);
    auto chieftains_house = assembleArea(nazchief);

    AreaDef shmaze;
    shmaze.name = "Shadow Maze";
    shmaze.type = AreaType::Structure;
    shmaze.parent = chieftains_house;
    shmaze.roomRanges.emplace_back(19400, 19499);
    auto shadow_maze = assembleArea(shmaze);

    AreaDef monbal;
    monbal.name = "Monastery of Balance";
    monbal.type = AreaType::Structure;
    monbal.parent = planet_konack;
    monbal.roomRanges.emplace_back(9500, 9599);
    monbal.roomRanges.emplace_back(9357, 9364);
    monbal.roomIDs.insert(9365);
    auto monastery_of_balance = assembleArea(monbal);

    AreaDef futschool;
    futschool.name = "Future School";
    futschool.parent = xenoverse;
    futschool.type = AreaType::Dimension;
    futschool.roomRanges.emplace_back(15938, 15999);
    auto future_school = assembleArea(futschool);

    AreaDef udfhq;
    udfhq.name = "UDF Headquarters";
    udfhq.parent = space;
    udfhq.type = AreaType::Structure;
    udfhq.roomRanges.emplace_back(18000, 18059);
    auto udf_headquarters = assembleArea(udfhq);

    AreaDef hspire;
    hspire.name = "The Haven Spire";
    hspire.parent = space;
    hspire.type = AreaType::Structure;
    hspire.roomRanges.emplace_back(18300, 18341);
    auto haven_spire = assembleArea(hspire);

    AreaDef knoit;
    knoit.name = "Kame no Itto";
    knoit.parent = space;
    knoit.type = AreaType::Structure;
    knoit.roomRanges.emplace_back(18400, 18460);
    auto kame_no_itto = assembleArea(knoit);

    AreaDef neonirvana;
    neonirvana.name = "Neo Nirvana";
    neonirvana.parent = space;
    neonirvana.type = AreaType::Structure;
    neonirvana.roomRanges.emplace_back(13500, 13552);
    neonirvana.roomRanges.emplace_back(14782, 14790);
    auto neo_nirvana = assembleArea(neonirvana);

    AreaDef neohologram;
    neohologram.name = "Hologram Combat";
    neohologram.parent = neo_nirvana;
    neohologram.roomRanges.emplace_back(13553, 13567);
    auto neo_hologram_combat = assembleArea(neohologram);

    AreaDef neonexusfield;
    neonexusfield.name = "Nexus Field";
    neonexusfield.parent = neo_hologram_combat;
    neonexusfield.roomRanges.emplace_back(13568, 13612);
    auto neo_nexus_field = assembleArea(neonexusfield);

    AreaDef neonamekgrassyisland;
    neonamekgrassyisland.name = "Namek: Grassy Island";
    neonamekgrassyisland.parent = neo_hologram_combat;
    neonamekgrassyisland.roomRanges.emplace_back(13613, 13657);
    auto neo_namek_grassy_island = assembleArea(neonamekgrassyisland);

    AreaDef neoslavemarket;
    neoslavemarket.name = "Slave Market";
    neoslavemarket.parent = neo_hologram_combat;
    neoslavemarket.roomRanges.emplace_back(13658, 13702);
    auto neo_slave_market = assembleArea(neoslavemarket);

    AreaDef neokanassa;
    neokanassa.name = "Kanassa: Blasted Battlefield";
    neokanassa.parent = neo_hologram_combat;
    neokanassa.roomRanges.emplace_back(13703, 13747);
    auto neo_kanassa_blasted_battlefield = assembleArea(neokanassa);

    AreaDef neosilentglade;
    neosilentglade.name = "Silent Glade";
    neosilentglade.parent = neo_hologram_combat;
    neosilentglade.roomRanges.emplace_back(13748, 13792);
    auto neo_silent_glade = assembleArea(neosilentglade);

    AreaDef neohell;
    neohell.name = "Hell - Flat Plains";
    neohell.parent = neo_hologram_combat;
    neohell.roomRanges.emplace_back(13793, 13837);
    auto neo_hell_flat_plains = assembleArea(neohell);

    AreaDef neosandydesert;
    neosandydesert.name = "Sandy Desert";
    neosandydesert.parent = neo_hologram_combat;
    neosandydesert.roomRanges.emplace_back(13838, 13882);
    auto neo_sandy_desert = assembleArea(neosandydesert);

    AreaDef neotopicasnowfield;
    neotopicasnowfield.name = "Topica Snowfield";
    neotopicasnowfield.parent = neo_hologram_combat;
    neotopicasnowfield.roomRanges.emplace_back(13883, 13927);
    auto neo_topica_snow_field = assembleArea(neotopicasnowfield);

    AreaDef neogerolab;
    neogerolab.name = "Gero's Lab";
    neogerolab.parent = neo_hologram_combat;
    neogerolab.roomRanges.emplace_back(13928, 14517);
    auto neo_geros_lab = assembleArea(neogerolab);

    AreaDef neocandyland;
    neocandyland.name = "Candy Land";
    neocandyland.parent = neo_hologram_combat;
    neocandyland.roomRanges.emplace_back(14518, 14562);
    auto neo_candy_land = assembleArea(neocandyland);

    AreaDef neoancestralmountains;
    neoancestralmountains.name = "Ancestral Mountains";
    neoancestralmountains.parent = neo_hologram_combat;
    neoancestralmountains.roomRanges.emplace_back(14563, 14607);
    auto neo_ancestral_mountains = assembleArea(neoancestralmountains);

    AreaDef neoelzthuanforest;
    neoelzthuanforest.name = "Elzthuan Forest";
    neoelzthuanforest.parent = neo_hologram_combat;
    neoelzthuanforest.roomRanges.emplace_back(14608, 14652);
    auto neo_elzthuan_forest = assembleArea(neoelzthuanforest);

    AreaDef neoyardracity;
    neoyardracity.name = "Yardra City";
    neoyardracity.parent = neo_hologram_combat;
    neoyardracity.roomRanges.emplace_back(14653, 14697);
    auto neo_yardra_city = assembleArea(neoyardracity);

    AreaDef neoancientcoliseum;
    neoancientcoliseum.name = "Ancient Coliseum";
    neoancientcoliseum.parent = neo_hologram_combat;
    neoancientcoliseum.roomRanges.emplace_back(14698, 14742);
    auto neo_ancient_coliseum = assembleArea(neoancientcoliseum);

    AreaDef fortrancomplex;
    fortrancomplex.name = "Fortran Complex";
    fortrancomplex.parent = neo_nirvana;
    fortrancomplex.roomRanges.emplace_back(14743, 14772);
    auto fortran_complex = assembleArea(fortrancomplex);

    AreaDef revolutionpark;
    revolutionpark.name = "Revolution Park";
    revolutionpark.parent = neo_nirvana;
    revolutionpark.roomRanges.emplace_back(14773, 14802);
    auto revolution_park = assembleArea(revolutionpark);

    AreaDef akatsukilabs;
    akatsukilabs.name = "Akatsuki Labs";
    akatsukilabs.parent = neo_nirvana;
    akatsukilabs.roomRanges.emplace_back(14800, 14893);
    auto akatsuki_labs = assembleArea(akatsukilabs);

    AreaDef southgal;
    southgal.name = "South Galaxy";
    southgal.parent = mortal_plane;
    southgal.roomIDs = {64300, 64399};
    auto south_galaxy = assembleArea(southgal);

    AreaDef undergroundpassage;
    undergroundpassage.name = "Underground Passage";
    undergroundpassage.parent = planet_namek;
    undergroundpassage.roomRanges.emplace_back(12869, 12899);
    auto underground_passage = assembleArea(undergroundpassage);

    AreaDef shatplan;
    shatplan.name = "Shattered Planet";
    shatplan.parent = south_galaxy;
    shatplan.type = AreaType::CelestialBody;
    shatplan.roomRanges.emplace_back(64301, 64399);
    auto shattered_planet = assembleArea(shatplan);

    AreaDef wzdef;
    wzdef.name = "War Zone";
    wzdef.parent = xenoverse;
    wzdef.type = AreaType::Structure;
    wzdef.roomRanges.emplace_back(17700, 17702);
    auto war_zone = assembleArea(wzdef);

    AreaDef corlight;
    corlight.name = "Corridor of Light";
    corlight.parent = war_zone;
    corlight.roomRanges.emplace_back(17703, 17722);
    auto corridor_of_light = assembleArea(corlight);

    AreaDef cordark;
    cordark.name = "Corridor of Darkness";
    cordark.parent = war_zone;
    cordark.roomRanges.emplace_back(17723, 17743);
    auto corridor_of_darkness = assembleArea(cordark);

    AreaDef soisland;
    soisland.name = "South Ocean Island";
    soisland.parent = planet_earth;
    soisland.roomRanges.emplace_back(6700, 6758);
    auto south_ocean_island = assembleArea(soisland);

    AreaDef hhouse;
    hhouse.name = "Haunted House";
    hhouse.parent = xenoverse;
    hhouse.type = AreaType::Dimension;
    hhouse.roomRanges.emplace_back(18600, 18693);
    auto haunted_house = assembleArea(hhouse);

    AreaDef roc;
    roc.name = "Random Occurences, WTF?";
    roc.parent = xenoverse;
    roc.type = AreaType::Dimension;
    roc.roomRanges.emplace_back(18700, 18776);
    auto random_occurences = assembleArea(roc);

    AreaDef galstrong;
    galstrong.name = "Galaxy's Strongest Tournament";
    galstrong.parent = space;
    galstrong.type = AreaType::Structure;
    galstrong.roomRanges.emplace_back(17875, 17894);
    auto galaxy_strongest_tournament = assembleArea(galstrong);

    AreaDef arwater;
    arwater.name = "Arena - Water";
    arwater.parent = galaxy_strongest_tournament;
    arwater.roomRanges.emplace_back(17800, 17824);
    auto arena_water = assembleArea(arwater);

    AreaDef arring;
    arring.name = "Arena - The Ring";
    arring.parent = galaxy_strongest_tournament;
    arring.roomRanges.emplace_back(17825, 17849);
    auto arena_ring = assembleArea(arring);

    AreaDef arsky;
    arsky.name = "Arena - In the Sky";
    arsky.parent = galaxy_strongest_tournament;
    arsky.roomRanges.emplace_back(17850, 17875);
    auto arena_sky = assembleArea(arsky);

    AreaDef stdef;
    stdef.name = "Structures";
    auto structures = assembleArea(stdef);

    AreaDef spdef;
    spdef.name = "Spaceships";
    auto spaceships = assembleArea(spdef);

    auto crunch_ship = [&](old_ship_data &data) {

        AreaDef sdata;
        sdata.name = data.name;
        sdata.roomIDs = data.vnums;
        sdata.type = AreaType::Vehicle;
        sdata.parent = spaceships;
        auto ship = assembleArea(sdata);
        auto &s = areas[ship];
        if(data.ship_obj) s.extraVn = data.ship_obj.value();

        return ship;
    };

    for(auto &sd : gships) {
        crunch_ship(sd);
    }

    for(auto &sd : customs) {
        crunch_ship(sd);
    }

    // A very luxurious player custom home
    AreaDef dunnoHouse;
    dunnoHouse.name = "Dunno's House";
    dunnoHouse.parent = xenoverse;
    dunnoHouse.roomIDs = {19009, 19010, 19011, 19012, 19013, 19014, 19015, 19016, 19017, 19018,
                          19019, 19020, 19021, 19022, 19023};
    auto dunno_house = assembleArea(dunnoHouse);

    // This looks like an unused old player home, seems like it's attached to Cherry Blossom Mountain?
    AreaDef mountainFortress;
    mountainFortress.name = "Mountaintop Fortress";
    mountainFortress.parent = xenoverse;
    mountainFortress.roomIDs = {19025, 19026, 19027, 19028, 19029, 19030, 19031, 19032,
                                19033, 19034, 19035, 19036, 19037, 19038, 19024};
    auto mountain_fortress = assembleArea(mountainFortress);

    // Personal Ships / Pods...
    for(auto vn = 45000; vn <= 45199; vn++) {
        auto ovn = vn + 1000;
        auto o = obj_proto.find(ovn);
        if(o == obj_proto.end()) continue;
        old_ship_data shipData;
        shipData.name = o->second.name;
        shipData.ship_obj = ovn;
        shipData.vnums.insert(vn);
        shipData.hatch_room = vn;
        auto v = crunch_ship(shipData);
    }

    AreaDef sphouses;
    sphouses.name = "Small Player Houses";
    sphouses.parent = structures;
    auto small_player_houses = assembleArea(sphouses);

    int count = 1;
    for(auto i = 18800; i != 18896; i += 4) {
        AreaDef house;
        house.name = fmt::format("Small Player House {}", count++);
        house.roomRanges.emplace_back(i, i+3);
        house.parent = small_player_houses;
        assembleArea(house);
    }

    AreaDef mdhouses;
    mdhouses.name = "Deluxe Player Houses";
    mdhouses.parent = structures;
    auto medium_player_houses = assembleArea(mdhouses);

    count = 1;
    for(auto i = 18900; i != 18995; i += 5) {
        AreaDef house;
        house.name = fmt::format("Deluxe Player House {}", count++);
        house.roomRanges.emplace_back(i, i+4);
        house.parent = medium_player_houses;
        assembleArea(house);
    }

    AreaDef lphouses;
    lphouses.name = "Excellent Player Houses";
    lphouses.parent = structures;
    auto large_player_houses = assembleArea(lphouses);

    count = 1;
    for(auto i = 19100; i != 19195; i += 5) {
        AreaDef house;
        house.name = fmt::format("Excellent Player House {}", count++);
        house.roomRanges.emplace_back(i, i+4);
        house.parent = large_player_houses;
        assembleArea(house);
    }

    AreaDef pdimen;
    pdimen.name = "Personal Pocket Dimensions";
    auto personal_dimensions = assembleArea(pdimen);

    int counter = 1;
    for(auto vn = 19800; vn <= 19899; vn++) {
        AreaDef pdim;
        pdim.name = "Personal Pocket Dimension " + std::to_string(counter++);
        pdim.parent = personal_dimensions;
        pdim.roomIDs.insert(vn);
        pdim.type = AreaType::Dimension;
        pdim.gravity = 1000.0;
        auto pd = assembleArea(pdim);
    }

    AreaDef misc;
    misc.name = "Miscellaneous";
    for(auto &[rv, room] : world) {
        if(!room.area) misc.roomIDs.insert(rv);
    }
    auto misc_area = assembleArea(misc);

    for(auto r : {
        300, 800, 1150, 1180, 1287, 1428, 1456, 1506, 1636, 1710, 19510, 2141, 13020, // earth vnums
    	4264, 4300, 4351, 4400, 4600, 4800, 5100, 5150, 5165, 5200, 5500, 4944, // frigid vnums
        8006, 8300, 8400, 8447, 8500, 8600, 8700, 8800, 8900, 8954, 9200, 9700, 9855, 9864, 9900, 9949, // konack
        2226, 2600, 2616, 2709, 2800, 2899, 2615, // vegeta
        11600, 10182, 10474, 13300, 10203, 10922, 11600, // namek
        12010, 12103, 12300, 12400, 12480, 12010, // Aether
        14008, 14100, 14200, 14300, // yardrat
        17531, 7950, 17420, // cerria
        3412, 3520, 19600, // zenith
        14904, 15655, // kanassa
        16009, 16544, 16600 // Arlia
    }) {
        ROOM_FLAGS(r).set(ROOM_LANDING);
    }
}

static std::vector<std::pair<std::string, vnum>> characterToAccount;

void migrate_accounts() {
	// user files are stored in <cwd>/user/<folder>/<file>.usr so we'll need to do recursive iteration using
    // C++ std::filesystem...

    auto path = std::filesystem::current_path() / "user";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No user directory found, skipping account migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".usr") continue;
        auto accFile = p.path().parent_path().filename().string();

        // Open file for reading...
        std::ifstream file(p.path());

        // Step 1: create an ID for this account...
        auto id = account_data::getNextID();

        // Now let's get a new account_data...
        auto &a = accounts[id];

        // Moving forward, we assume that every account file is using the above structure and is valid.
        // Don't second-guess it, just process.

        // Line 1: Name (string)
        std::getline(file, a.name);

        // Line 2: Email Address (string)
        std::getline(file, a.email);

        // Line 3: password (clear text, will hash...)
        std::string pass;
        std::getline(file, pass);
        if(!a.setPassword(pass)) basic_mud_log("Error hashing %s's password: %s", a.name.c_str(), pass.c_str());

        // Line 4: slots (int)
        std::string slots;
        std::getline(file, slots);
        a.slots = std::stoi(slots);

        // Line 5: current RPP (int)
        std::string rpp;
        std::getline(file, rpp);
        a.rpp = std::stoi(rpp);

        // Now for the Character lines, they either contain a name, or they contain "Empty".
        // "Empty" is not a character. It's a placeholder for an empty slot.
        // For now, we will move through all five lines, and for any that do not contain "Empty",
        // we insert the character name into the characterToAccount map.

        for(int i = 0;i < 5; i++) {
            std::string charName;
            std::getline(file, charName);
            if(charName != "Empty") {
                characterToAccount.emplace_back(charName, id);
            }
        }

        // Line 11: adminLevel (int)
        std::string adminLevel;
        std::getline(file, adminLevel);
        a.adminLevel = std::stoi(adminLevel);

        // Line 12: customFile present (bool)
        std::string customFile;
        std::getline(file, customFile);
        bool custom = std::stoi(customFile);

        // if custom, then we want to open a sister file that ends in .cus in the same directory and read every line
        // beyond the first into a.customs.
        if(custom) {
            auto customPath = p.path();
            customPath.replace_extension(".cus");
            std::ifstream customFile(customPath);
            std::string line;
            std::getline(customFile, line); // skip the first line
            while(std::getline(customFile, line)) {
                a.customs.emplace_back(line);
            }
            customFile.close();
        }

        // Line 13: RPP bank (unused)
        std::string rppBank;
        std::getline(file, rppBank);
        auto bank = std::stoi(rppBank);
        file.close();
        a.vn = id;

    }
}

void migrate_characters() {
    // Unlike accounts, player files are indexed. However, unless their name showed up in an account,
    // there's no point migrating them.

    // The procedure we will use is: iterate through characterToAccount and attempt to load the character.
    // if we can load them, we'll convert them and bind them to the appropriate account.

    for(auto &[cname, accID] : characterToAccount) {
        auto ch = new char_data();
        if(load_char(cname.c_str(), ch) < 0) {
            basic_mud_log("Error loading %s for account migration.", cname.c_str());
            delete ch;
            continue;
        }
        auto id = ch->id;
        auto &p = players[id];
        p.id = id;
        if(!ch->generation) ch->generation = time(nullptr);
        p.character = ch;
        p.name = ch->name;
        auto &a = accounts[accID];
        p.account = &a;
        a.characters.emplace_back(id);
        ch->in_room = ch->load_room;
        ch->was_in_room = ch->load_room;
        uniqueCharacters[id] = std::make_pair(ch->generation, ch);
    }

    // migrate sense files...
    auto path = std::filesystem::current_path() / "sense";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No sense directory found, skipping account migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".sen") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for sense migration.", name.c_str());
            continue;
        }
        auto &pa = players[ch->id];
        // The file contains a sequence of lines, with each line containing a number.
		// The number is the vnum of a mobile the player's sensed.
        // We will read each line and insert the vnum into the player's sensed list.
        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            try {
                auto vnum = std::stoi(line);
                if(mob_proto.contains(vnum)) pa.senseMemory.insert(vnum);
            } catch(...) {
                basic_mud_log("Error parsing %s for sense migration.", line.c_str());
            }
        }
        file.close();
    }

    path = std::filesystem::current_path() / "intro";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No intro directory found, skipping intro migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".itr") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for dub migration.", name.c_str());
            continue;
        }

        auto &pa = players[ch->id];

		// The file contains a series of lines.
        // Each line looks like: <name> <dub>
        // We will need to use findPlayer on name, and then save id->dub to ch->player_specials->dubNames.
        // ignore if <name> == "Gibbles"

        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            auto pos = line.find(' ');
            if(pos == std::string::npos) continue;
            auto name = line.substr(0, pos);
            auto dub = line.substr(pos + 1);
            if(name == "Gibbles") continue;
            auto pc = findPlayer(name);
            if(!pc) continue;
            pa.dubNames[pc->id] = dub;
        }
    }

    path = std::filesystem::current_path() / "plrvars";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No intro directory found, skipping intro migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".mem") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for variable migration.", name.c_str());
            continue;
        }

        // The file contains a series of lines.
        // each line looks like this: <varname> <context> <data>
        // where varname is a single-word string, context is an integer, and data is a string - although it might be an
        // empty string.
        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            auto pos = line.find(' ');
            if(pos == std::string::npos) continue;
            auto varname = line.substr(0, pos);
            auto pos2 = line.find(' ', pos + 1);
            if(pos2 == std::string::npos) continue;
            auto context = line.substr(pos + 1, pos2 - pos - 1);
            auto data = line.substr(pos2 + 1);
            if(!ch->script) {
                ch->script = new script_data(ch);
            }

            try {
                auto ctx = std::stoi(context);
                add_var(&ch->script->global_vars, (char*)varname.c_str(), data.c_str(), ctx);
            } catch(...) {
                basic_mud_log("Error parsing %s for variable migration.", line.c_str());
            }
        }
    }

    path = std::filesystem::current_path() / "plralias";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No plralias directory found, skipping alias migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".alias") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for alias migration.", name.c_str());
            continue;
        }
        auto pa = players.find(ch->id);
        if(pa == players.end()) {
            basic_mud_log("Error loading %s for alias migration.", name.c_str());
            continue;
        }


        std::ifstream file(p.path());
        std::string line;

        // Alias files are stored as sequences of this:
        // "%d\n%s\n%d\n%s\n%d\n", in order: alias name string length (size_t), alias name string,
        // replacement string length  (size_t), replacement string, alias type (a bool)

        while(std::getline(file, line)) {
            auto &a = pa->second.aliases.emplace_back();
            std::getline(file, a.name);
            std::getline(file, line);
            std::getline(file, a.replacement);
            std::getline(file, line);
            a.type = atoi(line.c_str());
        }
    }

    path = std::filesystem::current_path() / "plrobjs";

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".new") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for object migration.", name.c_str());
            continue;
        }

		auto result = Crash_load(ch);

    }
}

void migrate_db() {

    migrate_grid();

    migrate_accounts();

    try {
        migrate_characters();
    } catch(std::exception &e) {
        basic_mud_log("Error migrating characters: %s", e.what());
    }
}

