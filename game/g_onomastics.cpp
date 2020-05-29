// g_onomastics.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_onomastics.h"

#include <set>

namespace game {
namespace onomastics {
namespace {

//------------------------------------------------------------------------------
const std::map<name_category, std::set<string::literal>> names = {
    { name_category::violent, {
        "apocalypse",
        "ambuscade",
        "ambush",
        "armageddon",
        "assault",
        "banshee",
        "barracuda",
        "battle",
        "behemoth",
        "brigantine",
        "conqueror",
        "conquest",
        "crusader",
        "destruction",
        "devastation",
        "eruption",
        "explosion",
        "fenrir",
        "fireball",
        "firebrand",
        "furious",
        "fury",
        "gladiator",
        "hornet",
        "hydra",
        "jormungandr",
        "kraken",
        "legion",
        "leviathan",
        "lion",
        "lioness",
        "ogre",
        "onslaught",
        "piercer",
        "pouncer",
        "puncher",
        "ragnorok",
        "raider",
        "skirmisher",
        "slinger",
        "smiter",
        "spitfire",
        "striker",
        "thrasher",
        "tiger",
        "tigress",
        "tornado",
        "torrent",
        "tyrant",
        "valkyrie",
        "violence",
        "violent",
        "wolf",
    }},
    { name_category::aggressive, {
        "aggressor",
        "alligator",
        "ambuscade",
        "ambush",
        "antagonist",
        "assault",
        "avenger",
        "banshee",
        "barracuda",
        "battle",
        "battleaxe",
        "bedlam",
        "blaze",
        "bloodhound",
        "brigantine",
        "bruiser",
        "challenger",
        "cobra",
        "combustion",
        "conqueror",
        "conquest",
        "convulsion",
        "crocodile",
        "crusader",
        "dangerous",
        "daring",
        "dasher",
        "demon",
        "dervish",
        "destruction",
        "devastation",
        "dragon",
        "eruption",
        "escapade",
        "exploit",
        "explosion",
        "falcon",
        "fencer",
        "fighter",
        "fireball",
        "firebrand",
        "flame",
        "fox",
        "foxhound",
        "furious",
        "fury",
        "gladiator",
        "goliath",
        "goshawk",
        "grappler",
        "growler",
        "harpy",
        "havoc",
        "hawk",
        "hornet",
        "hostile",
        "hound",
        "hunter",
        "hydra",
        "impulsive",
        "incendiary",
        "infernal",
        "insolent",
        "jaguar",
        "leopard",
        "lightning",
        "lion",
        "lioness",
        "menace",
        "mischief",
        "mongoose",
        "nighthawk",
        "onslaught",
        "osprey",
        "panther",
        "piercer",
        "polecat",
        "pouncer",
        "puncher",
        "pursuer",
        "python",
        "raider",
        "revenge",
        "roc",
        "savage",
        "scorcher",
        "scourge",
        "serpent",
        "shark",
        "skirmisher",
        "slinger",
        "smiter",
        "snake",
        "sparrowhawk",
        "spitfire",
        "stalker",
        "striker",
        "tempest",
        "terror",
        "thrasher",
        "thruster",
        "thunderbolt",
        "thunderer",
        "tiger",
        "tigress",
        "tormentor",
        "tornado",
        "torrent",
        "tracker",
        "trespasser",
        "trooper",
        "uproar",
        "vanquisher",
        "vengeance",
        "vindictive",
        "violent",
        "viper",
        "virulent",
        "warrior",
        "warspite",
        "wasp",
        "wolf",
        "wolfhound",
        "wolverine",
        "wrangler",
        "wyvern",
    }},
    { name_category::evil, {
        "apocalypse",
        "armageddon",
        "assiduous",
        "banshee",
        "convulsion",
        "demon",
        "devastation",
        "devil",
        "devilfish",
        "dictator",
        "disdain",
        "dreadful",
        "hel",
        "infernal",
        "jormungandr",
        "leviathan",
        "lucifer",
        "malice",
        "scourge",
        "spiteful",
        "terror",
        "tormentor",
        "tyrant",
        "vampire",
        "virulent",
        "witch",
    }},
    { name_category::heroic, {
        "adamant",
        "admirable",
        "arbiter",
        "ardent",
        "assistance",
        "assurance",
        "audacious",
        "austere",
        "bastion",
        "beacon",
        "brave",
        "brisk",
        "bulwark",
        "buttress",
        "cavalier",
        "celerity",
        "challenger",
        "champion",
        "chivalrous",
        "citadel",
        "courageous",
        "crusader",
        "daring",
        "dasher",
        "dauntless",
        "defence",
        "defender",
        "defiance",
        "destiny",
        "diligence",
        "diligent",
        "dreadnought",
        "earnest",
        "endeavour",
        "endurance",
        "enterprise",
        "escort",
        "excalibur",
        "faith",
        "faithful",
        "fearless",
        "formidable",
        "fortitude",
        "gallant",
        "generous",
        "glorious",
        "glory",
        "grace",
        "griffin",
        "guardian",
        "hardy",
        "hearty",
        "herald",
        "hero",
        "honesty",
        "hope",
        "immortal",
        "implacable",
        "impregnable",
        "indefatigable",
        "integrity",
        "intelligence",
        "intrepid",
        "invincible",
        "irresistable",
        "lion",
        "lioness",
        "lively",
        "loyal",
        "loyalty",
        "mentor",
        "mindful",
        "monarch",
        "noble",
        "obdurate",
        "obedient",
        "paladin",
        "paragon",
        "patroller",
        "persistence",
        "persistant",
        "phoenix",
        "prince",
        "princess",
        "prosperity",
        "prosperous",
        "protector",
        "prudence",
        "prudent",
        "queen",
        "recovery",
        "renown",
        "repulse",
        "resistance",
        "resolute",
        "resolution",
        "restoration",
        "restless",
        "rigorous",
        "robust",
        "royal",
        "royalist",
        "safeguard",
        "safety",
        "sanguine",
        "security",
        "smiter",
        "sovereign",
        "splendid",
        "stalwart",
        "standard",
        "staunch",
        "steadfast",
        "strenuous",
        "stronghold",
        "sturdy",
        "supreme",
        "templar",
        "tenacious",
        "tenacity",
        "tiger",
        "tigress",
        "trusty",
        "unbeaten",
        "unbending",
        "unbroken",
        "undaunted",
        "unity",
        "unswerving",
        "untiring",
        "upholder",
        "upright",
        "valiant",
        "valkyrie",
        "valorous",
        "vanquisher",
        "venerable",
        "victor",
        "vivacious",
        "warrior",
        "watchman",
        "wolfhound",
        "zealous",
    }},
    { name_category::villainous, {
        "aggressor",
        "ambuscade",
        "ambush",
        "antagonist",
        "arrogance",
        "assault",
        "assiduous",
        "banshee",
        "bedlam",
        "brigantine",
        "bruiser",
        "caprice",
        "dangerous",
        "demon",
        "dervish",
        "desire",
        "destruction",
        "devastation",
        "dictator",
        "disdain",
        "dragon",
        "dreadful",
        "exploit",
        "explosion",
        "gladiator",
        "gore",
        "harrow",
        "haughty",
        "havoc",
        "hydra",
        "impulsive",
        "incendiary",
        "infernal",
        "insolent",
        "locust",
        "loki",
        "lucifer",
        "malice",
        "mantis",
        "maroon",
        "menace",
        "mischief",
        "mosquito",
        "nemesis",
        "onslaught",
        "raider",
        "revenge",
        "savage",
        "serpent",
        "shark",
        "spiteful",
        "terrible",
        "terror",
        "tormentor",
        "trespasser",
        "truculent",
        "tyrant",
        "unbridled",
        "unruly",
        "uproar",
        "usurper",
        "vampire",
        "vendetta",
        "vengeance",
        "venomous",
        "vindictive",
        "violent",
        "viper",
        "vulture",
        "weasel",
        "wolf",
    }},
    { name_category::resistance, {
        "ambush",
        "antagonist",
        "audacious",
        "avenger",
        "brigantine",
        "challenger",
        "combatant",
        "contest",
        "courageous",
        "dangerous",
        "daring",
        "dasher",
        "dauntless",
        "defence",
        "defender",
        "defiance",
        "endeavour",
        "endurance",
        "fearless",
        "fervent",
        "fighter",
        "hope",
        "insolent",
        "lookout",
        "loyal",
        "loyalty",
        "nemesis",
        "obdurate",
        "persistence",
        "persistant",
        "phoenix",
        "prudence",
        "prudent",
        "raider",
        "recovery",
        "resistance",
        "resolute",
        "resolution",
        "restoration",
        "restless",
        "revenge",
        "safeguard",
        "safety",
        "sentinel",
        "skirmisher",
        "stalwart",
        "staunch",
        "steadfast",
        "strenuous",
        "stronghold",
        "tenacious",
        "tenacity",
        "thistle",
        "thorn",
        "truculent",
        "unbeaten",
        "unbending",
        "unbridled",
        "unbroken",
        "undaunted",
        "unswerving",
        "untamed",
        "untiring",
        "uproar",
        "valiant",
        "vigilant",
        "wakeful",
        "watchman",
    }},
    { name_category::peaceful, {
        "abundance",
        "adventure",
        "alacrity",
        "alliance",
        "ambassador",
        "arbiter",
        "ascension",
        "assistance",
        "assurance",
        "attentive",
        "austere",
        "brisk",
        "celerity",
        "charity",
        "crown",
        "constitution",
        "courier",
        "destiny",
        "diamond",
        "director",
        "discovery",
        "earnest",
        "echo",
        "eclipse",
        "emerald",
        "encounter",
        "endeavour",
        "endurance",
        "enterprise",
        "escort",
        "excellent",
        "explorer",
        "extravagant",
        "faith",
        "faithful",
        "flash",
        "flight",
        "fortune",
        "forward",
        "fountain",
        "friendship",
        "garnet",
        "generous",
        "grace",
        "greyhound",
        "handy",
        "hearty",
        "herald",
        "honesty",
        "hope",
        "implacable",
        "indefatigable",
        "inspector",
        "integrity",
        "intelligence",
        "intrepid",
        "invention",
        "investigator",
        "irresistable",
        "jasmine",
        "jewel",
        "jolly",
        "joyful",
        "jubilant",
        "laurel",
        "lavender",
        "lively",
        "magnet",
        "marathon",
        "mentor",
        "mindful",
        "mystic",
        "nimble",
        "noble",
        "nomad",
        "obedient",
        "observer",
        "onyx",
        "opal",
        "oracle",
        "otter",
        "paragon",
        "paramour",
        "peace",
        "pegasus",
        "persistence",
        "persistant",
        "premier",
        "president",
        "proctor",
        "prosperity",
        "prosperous",
        "prudence",
        "prudent",
        "quail",
        "quality",
        "quest",
        "racer",
        "rainbow",
        "rapid",
        "regent",
        "renown",
        "reward",
        "rigorous",
        "robust",
        "rose",
        "royal",
        "royalist",
        "ruby",
        "sapphire",
        "scout",
        "seagull",
        "seahorse",
        "searcher",
        "senator",
        "sentinel",
        "serene",
        "sleuth",
        "sovereign",
        "speaker",
        "spirit",
        "splendid",
        "sportive",
        "standard",
        "sterling",
        "success",
        "swift",
        "thorough",
        "topaz",
        "torch",
        "trailer",
        "transfer",
        "transit",
        "traveller",
        "tribune",
        "trusty",
        "turquoise",
        "umbra",
        "unique",
        "unite",
        "unity",
        "upholder",
        "upright",
        "upward",
        "utmost",
        "utopia",
        "venerable",
        "venturous",
        "versatile",
        "vivacious",
        "voyager",
        "wager",
        "walker",
        "wanderer",
        "wizard",
        "zephyr",
        "zest",
    }},
    { name_category::mythical, {
        "banshee",
        "basilisk",
        "behemoth",
        "demon",
        "devil",
        "dragon",
        "excalibur",
        "goliath",
        "griffin",
        "harpy",
        "isis",
        "kraken",
        "leviathan",
        "lucifer",
        "mermaid",
        "osiris",
        "roc",
        "titania",
        "valkyrie",
        "vampire",
        "witch",
        "wizard",
        "wyvern",
    }},
    { name_category::greek_mythology, {
        "canopus",
        "cerberus",
        "cyclops",
        "daedalus",
        "europa",
        "ganymede",
        "hydra",
        "icarus",
        "medusa",
        "minotaur",
        "narcissus",
        "nymph",
        "orion",
        "pegasus",
        "perseus",
        "phoenix",
        "pollux",
        "prometheus",
        "satyr",
        "sphinx",
        "theseus",
        "triton",
    }},
    { name_category::roman_mythology, {
        "pluto",
        "pollux",
        "taurus",
        "ursa",
    }},
    { name_category::greek_pantheon, {
        "aphrodite",
        "apollo",
        "artemis",
        "athena",
        "demeter",
        "dionysus",
        "hephaestus",
        "hera",
        "hermes",
        "hestia",
        "poseidon",
        "zeus",
    }},
    { name_category::roman_pantheon, {
        "apollo",
        "bacchus",
        "ceres",
        "diana",
        "juno",
        "jupiter",
        "mars",
        "minerva",
        "neptune",
        "venus",
        "vesta",
        "vulcan",
    }},
    { name_category::norse_mythology, {
        "fenrir",
        "freya",
        "hel",
        "jormungandr",
        "loki",
        "odin",
        "ragnorok",
        "thor",
    }},
    { name_category::creature, {
        "alligator",
        "auroch",
        "banshee",
        "basilisk",
        "behemoth",
        "bloodhound",
        "cerberus",
        "chameleon",
        "cobra",
        "crocodile",
        "cyclops",
        "demon",
        "devil",
        "dragon",
        "eagle",
        "elephant",
        "falcon",
        "fenrir",
        "fox",
        "foxhound",
        "gazelle",
        "goshawk",
        "griffin",
        "harpy",
        "hawk",
        "hind",
        "hornet",
        "hound",
        "hydra",
        "jackdaw",
        "jaguar",
        "kangaroo",
        "kestrel",
        "kingfisher",
        "ladybird",
        "lark",
        "leopard",
        "lion",
        "lizard",
        "locust",
        "lynx",
        "magpie",
        "mandrake",
        "mantis",
        "mastiff",
        "medusa",
        "midge",
        "minotaur",
        "moa",
        "mongoose",
        "monitor",
        "monkey",
        "mosquito",
        "moth",
        "nighthawk",
        "nightingale",
        "nymph",
        "ocelot",
        "ogre",
        "opossum",
        "osprey",
        "ostrich",
        "otter",
        "owl",
        "panther",
        "parrot",
        "pegasus",
        "pelican",
        "penguin",
        "pheasant",
        "phoenix",
        "pigeon",
        "platypus",
        "polecat",
        "porcupine",
        "puffin",
        "puma",
        "quail",
        "raccoon",
        "rattlesnake",
        "reindeer",
        "roc",
        "salamander",
        "scarab",
        "scorpion",
        "seagull",
        "seahawk",
        "serpent",
        "siren",
        "skylark",
        "snake",
        "sparrow",
        "sparrowhawk",
        "sphinx",
        "spider",
        "starling",
        "stork",
        "swan",
        "taurus",
        "tiger",
        "tigress",
        "unicorn",
        "ursa",
        "vampire",
        "viper",
        "walrus",
        "wasp",
        "weasel",
        "witch",
        "wizard",
        "wolf",
        "wolfhound",
        "wolverine",
        "wren",
        "wyvern",
        "zebra",
    }},
    { name_category::humanoid, {
        "aphrodite",
        "apollo",
        "artemis",
        "athena",
        "bacchus",
        "banshee",
        "ceres",
        "cyclops",
        "daedalus",
        "demeter",
        "demon",
        "dervish",
        "diana",
        "dionysus",
        "europa",
        "freya",
        "harpy",
        "hephaestus",
        "hel",
        "hera",
        "hermes",
        "hestia",
        "isis",
        "juno",
        "jupiter",
        "loki",
        "lucifer",
        "mars",
        "medusa",
        "mermaid",
        "minerva",
        "minotaur",
        "neptune",
        "nymph",
        "odin",
        "ogre",
        "orion",
        "osiris",
        "perseus",
        "pollux",
        "poseidon",
        "prometheus",
        "romulus",
        "saturn",
        "siren",
        "sphinx",
        "theseus",
        "thor",
        "titania",
        "triton",
        "venus",
        "vesta",
        "vulcan",
        "witch",
        "wizard",
        "zeus",
    }},
    { name_category::sea_creature, {
        "barracuda",
        "devilfish",
        "grouper",
        "jormungandr",
        "kingfish",
        "kraken",
        "leviathan",
        "mermaid",
        "salmon",
        "sardine",
        "seahorse",
        "sealion",
        "skate",
        "shark",
        "snapper",
        "spearfish",
        "sturgeon",
        "starfish",
        "sunfish",
        "swordfish",
        "tuna",
    }},
    { name_category::weapon, {
        "battleaxe",
        "blade",
        "boomerang",
        "broadsword",
        "carronade",
        "claymore",
        "crossbow",
        "culverin",
        "cutlass",
        "dagger",
        "excalibur",
        "grenade",
        "howitzer",
        "javelin",
        "lance",
        "longbow",
        "morning star",
        "mortar",
        "musket",
        "petard",
        "pike",
        "rocket",
        "scimitar",
        "scythe",
        "sickle",
        "spear",
        "spearhead",
        "strongbow",
        "trident",
        "truncheon",
    }},
};

//------------------------------------------------------------------------------
const std::map<name_category, std::set<name_category>> subcategories = {
    { name_category::violent, {
        name_category::weapon,
    }},
    { name_category::aggressive, {
        name_category::weapon,
    }},
    { name_category::humanoid, {
        name_category::greek_pantheon,
        name_category::roman_pantheon,
    }},
    { name_category::mythical, {
        name_category::greek_mythology,
        name_category::roman_mythology,
        name_category::norse_mythology,
    }},
    { name_category::greek_mythology, {
        name_category::greek_pantheon,
    }},
    { name_category::roman_mythology, {
        name_category::roman_pantheon,
    }},
    { name_category::creature, {
        name_category::sea_creature,
    }},
};

//------------------------------------------------------------------------------
std::set<name_category> expand_category(name_category c);
std::set<name_category> expand_categories(std::set<name_category> const& in)
{
    std::set<name_category> out = in;
    for (auto const& c : in) {
        auto expanded = expand_category(c);
        out.insert(expanded.begin(), expanded.end());
    }
    return out;
}

//------------------------------------------------------------------------------
std::set<name_category> expand_category(name_category c)
{
    auto it = subcategories.find(c);
    if (it != subcategories.cend()) {
        return expand_categories(it->second);
    } else {
        return {};
    }
}

//------------------------------------------------------------------------------
std::set<string::literal> enumerate_categories(std::set<name_category> const& s)
{
    std::set<string::literal> out;
    for (auto const& c : s) {
        auto next = expand_categories({c});
        for (auto const& n : next) {
            auto it = names.find(n);
            if (it != names.cend()) {
                out.insert(it->second.begin(), it->second.end());
            }
        }
    }
    return out;
}

//------------------------------------------------------------------------------
std::set<string::literal> intersect_categories(std::set<name_category> const& s)
{
    std::vector<string::literal> out;

    if (!s.size()) {
        return std::set<string::literal>();
    }

    {
        auto init = enumerate_categories(expand_categories({*s.begin()}));
        out = std::vector<string::literal>(init.begin(), init.end());
    }

    for (auto const& c : s) {
        if (!out.size()) {
            break;
        }

        auto next = enumerate_categories(expand_categories({c}));
        std::vector<string::literal> intersect;
        std::set_intersection(
            out.begin(),
            out.end(),
            next.begin(),
            next.end(),
            std::back_inserter(intersect),
            std::less<string::literal>());
        out = std::move(intersect);
    }

    return std::set<string::literal>(out.begin(), out.end());
}

} // anonymous namespace

//------------------------------------------------------------------------------
string::view random_name_including(
    random& r,
    name_category const* include,
    std::size_t include_size,
    name_category const* exclude,
    std::size_t exclude_size)
{
    std::set<string::literal> include_names = enumerate_categories({include, include + include_size});
    std::set<string::literal> exclude_names = enumerate_categories({exclude, exclude + exclude_size});

    std::vector<string::literal> working_set;
    std::set_difference(
        include_names.cbegin(),
        include_names.cend(),
        exclude_names.cbegin(),
        exclude_names.cend(),
        std::back_inserter(working_set),
        std::less<string::literal>());

    if (working_set.size()) {
        return working_set[r.uniform_int(working_set.size())];
    } else {
        return "";
    }
}

//------------------------------------------------------------------------------
string::view random_name_requiring(
    random& r,
    name_category const* require,
    std::size_t require_size,
    name_category const* exclude,
    std::size_t exclude_size)
{
    std::set<string::literal> require_names = intersect_categories({require, require + require_size});
    std::set<string::literal> exclude_names = enumerate_categories({exclude, exclude + exclude_size});

    std::vector<string::literal> working_set;
    std::set_difference(
        require_names.cbegin(),
        require_names.cend(),
        exclude_names.cbegin(),
        exclude_names.cend(),
        std::back_inserter(working_set),
        std::less<string::literal>());

    if (working_set.size()) {
        return working_set[r.uniform_int(working_set.size())];
    } else {
        return "";
    }
}

string::view foo()
{
    random r;
    return random_name_including(r, {name_category::evil, name_category::ugly}, {name_category::mythical});
}

string::view bar()
{
    random r;
    return random_name_requiring(r, {name_category::evil, name_category::ugly}, {name_category::mythical});
}

//------------------------------------------------------------------------------
string::literal name_category_name(name_category c)
{
#define CASE(c) case name_category::c: return string::literal(#c)
    switch (c) {
        CASE(violent);
        CASE(evil);
        CASE(ugly);
        CASE(pretty);
        CASE(heroic);
        CASE(villainous);
        CASE(resistance);
        CASE(luxurious);
        CASE(aggressive);
        CASE(peaceful);
        CASE(protector);
        CASE(creature);
        CASE(humanoid);
        CASE(sea_creature);
        CASE(mythical);
        CASE(greek_mythology);
        CASE(roman_mythology);
        CASE(norse_mythology);
        CASE(greek_pantheon);
        CASE(roman_pantheon);
        CASE(weapon);
    }
#undef CASE
    return "";
}

//------------------------------------------------------------------------------
void check_name_overlap()
{
    auto const colorize = [](float t) {
        if (t > 1.f) {
            return string::view("^fff");
        } else if (t > .5f) {
            return va("^ff%x", clamp<int>((t - .5f) * 32.f, 0, 15));
        } else if (t > 0.f) {
            return va("^f%x0", clamp<int>(t * 32.f, 0, 15));
        } else {
            return string::view("^f00");
        }
    };

    struct result {
        name_category c1;
        name_category c2;
        float disjoint;
        float overlap;
        std::set<string::literal> intersection;

        bool operator<(result const& rhs) const {
            return overlap > rhs.overlap;
        }
    };

    std::vector<result> results;

    for (auto const& kv1 : names) {
        for (auto const& kv2 : names) {
            if (int(kv1.first) >= int(kv2.first)) {
                continue;
            }
            auto s1 = expand_categories({kv1.first});
            auto s2 = expand_categories({kv2.first});

            // Skip if either name category is a subcategory of the other
            if (s1.find(kv2.first) != s1.cend() || s2.find(kv1.first) != s2.cend()) {
                continue;
            }

            auto n1 = enumerate_categories({kv1.first});
            auto n2 = enumerate_categories({kv2.first});

            auto total = enumerate_categories({kv1.first, kv2.first});
            auto intersection = intersect_categories({kv1.first, kv2.first});

            if (!intersection.size()) {
                continue;
            }

            results.push_back({
                n1.size() < n2.size() ? kv1.first : kv2.first,
                n1.size() < n2.size() ? kv2.first : kv1.first,
                (total.size() - intersection.size()) / float(total.size()),
                (intersection.size()) / float(min(n1.size(), n2.size())),
                std::move(intersection)
            });
        }
    }

    std::sort(results.begin(), results.end());
    for (auto const& r : results) {
        log::message("%s%3.0f^xxx%% overlap %s%3.0f^xxx%% disjoint (^fff%s^xxx|^fff%s^xxx)\n",
            colorize(1.f - r.overlap).c_str(),
            r.overlap * 100.,
            colorize(r.disjoint).c_str(),
            r.disjoint * 100.,
            name_category_name(r.c1).c_str(),
            name_category_name(r.c2).c_str());
        log::message("    ");
        int num = 0;
        for (auto const& n : r.intersection) {
            if (++num > 8) {
                log::message("...");
                break;
            }
            log::message("%s ", n.c_str());
        }
        log::message("\n");
    }
}

} // namespace onomastics
} // namespace game
