#include "stdafx.h"
#include "dota.h"
#include "utils.h"

static DotaHero heroes[] = {
  {-1, '0000', "No Hero", "", 0, "", "No Hero"},
  {0, 'Hvwd', "Vengeful Spirit", "Shendelzare Silkwood", 1657, "BTNAvengingWatcher", "Vengeful"},
  {0, 'Hmbr', "Lord of Olympia", "Zeus", 1653, "BTNAvatar", "Zeus"},
  {0, 'Emoo', "Enchantress", "Aiushtha", 1632, "BTNDryad", "Aiushta"},
  {0, 'O00P', "Morphling", "Morphling", 1690, "BTNSeaElemental", "Morphling"},
  {0, 'Hjai', "Crystal Maiden", "Rylai Crestfallen", 1651, "BTNJaina", "Rylai"},
  {0, 'H001', "Rogue Knight", "Sven", 1703, "BTNFelGuardBlue", "Sven"},
  {0, 'HC49', "Naga Siren", "Slithice", 1682, "BTNSeaWitch", "Slithice"},
  {0, 'Otch', "Earthshaker", "Raigor Stonehoof", 1628, "BTNTauren", "Raigor"},
  {0, 'HC92', "Stealth Assasin", "Rikimaru", 1684, "BTNSatyrTrickster", "Rikimaru"},
  {0, 'N01O', "Lone Druid", "Syllabear", 1686, "BTNDruidOfTheClaw", "Syllabear"},
  {0, 'H004', "Slayer", "Lina Inverse", 1708, "BTNSorceress", "Lina"},
  {0, 'Nbbc', "Juggernaut", "Yurnero", 1659, "BTNChaosBlademaster", "Yurnero"},

  {1, 'N01A', "Silencer", "Nortrom", 1707, "BTNSpellBreaker", "Nortrom"},
  {1, 'Hamg', "Treant Protector", "Rooftrellen", 1621, "BTNTreant", "Treant"},
  {1, 'Uktl', "Enigma", "Darchrow", 1665, "BTNSpell_Shadow_SummonVoidWalker", "Enigma"},
  {1, 'Hblm', "Keeper of the Light", "Ezalor", 1622, "BTNGhostMage", "Ezalor"},
  {1, 'Huth', "Ursa Warrior", "Ulfsaar", 1655, "BTNFurbolgTracker", "Ursa"},
  {1, 'Hmkg', "Ogre Magi", "Aggron Stonebreaker", 1623, "BTNOgreMagi", "Ogre"},
  {1, 'Ntin', "Tinker", "Boush", 1644, "BTNHeroTinker", "Tinker"},
  {1, 'Emns', "Prophet", "Furion", 1647, "BTNFurion", "Furion"},
  {1, 'Ogrh', "Phantom Lancer", "Azwraith", 1660, "BTNHellScream", "Azwraith"},
  {1, 'Ucrl', "Stone Giant", "Tiny", 1634, "BTNMountainGiant", "Tiny"},
  {1, 'H00K', "Goblin Techies", "Squee and Spleen", 1680, "BTNGoblinSapper", "Techies"},
  {1, 'H00A', "Holy Knight", "Chen", 1711, "BTNHeroFarseer", "Chen"},

  {2, 'E005', "Moon Rider", "Luna Moonfang", 1702, "BTNHuntress", "Luna"},
  {2, 'Usyl', "Dwarven Sniper", "Kardel Sharpeye", 1666, "BTNRifleman", "Sniper"},
  {2, 'N016', "Troll Warlord", "Jah'rakal", 1674, "BTNForestTroll", "Jahrakal"},
  {2, 'Orkn', "Shadow Shaman", "Rhasta", 1662, "BTNShadowHunter", "Rhasta"},
  {2, 'H008', "Bristleback", "Rigwarl", 1705, "BTNRazorManeChief", "Rigwarl"},
  {2, 'Npbm', "Pandaren Brewmaster", "Mangix", 1642, "BTNPandarenBrewmaster", "Mangix"},
  {2, 'H000', "Centaur Warchief", "Bradwarden", 1689, "BTNCentaurKhan", "Centaur"},
  {2, 'Naka', "Bounty Hunter", "Gondar", 1658, "BTNakama", "Gondar"},
  {2, 'Hlgr', "Dragon Knight", "Knight Davion", 1652, "BTNTheCaptain", "Davion"},
  {2, 'Edem', "Anti-Mage", "Magina", 1629, "BTNHeroDemonHunter", "Magina"},
  {2, 'Nbrn', "Drow Ranger", "Traxex", 1640, "BTNBansheeRanger", "Traxex"},
  {2, 'Harf', "Omniknight", "Purist Thunderwrath", 1649, "BTNArthas", "Purist"},

  {3, 'H00D', "Beastmaster", "Rexxar", 1715, "BTNBeastMaster", "Rexxar"},
  {3, 'E00P', "Twin Head Dragon", "Jakiro", 1716, "BTNChimaera", "Jakiro"},
  {3, 'N01I', "Alchemist", "Razzil Darkbrew", 1720, "BTNHeroAlchemist", "Razzil"},
  {3, 'N01V', "Priestess of the Moon", "Mirana Nightshade", 1723, "BTNPriestessOfTheMoon", "Mirana"},
  {3, 'H00S', "Storm Spirit", "Raijin", 1733, "BTNStorm", "Raijin"},
  {3, 'H00Q', "Sacred Warrior", "Huskar", 1730, "BTNHeadHunterBerserker", "Huskar"},
  {3, 'E01Y', "Templar Assasin", "Lanaya", 1734, "BTNAssassin", "Lanaya"},
  {3, 'N00B', "Faerie Dragon", "Puck", 1735, "BTNFaerieDragon", "Puck"},

  {4, 'Eevi', "Soul Keeper", "Terrorblade", 1645, "BTNEvilIllidan", "Terrorblade"},
  {4, 'Ekee', "Tormented Soul", "Leshrac the Malicious", 1631, "BTNKeeperGhostBlue", "Leshrac"},
  {4, 'Ulic', "Lich", "Kel'Thusad", 1637, "BTNLichVersion2", "Lich"},
  {4, 'UC76', "Death Prophet", "Krobelus", 1700, "BTNBanshee", "Krobelus"},
  {4, 'UC18', "Demon Witch", "Lion", 1672, "BTNLion", "Lion"},
  {4, 'EC57', "Venomancer", "Lesale Deathbringer", 1667, "BTNHydralisk", "Lesale"},
  {4, 'UC11', "Magnataur", "Magnus", 1691, "BTNBlueMagnataur", "Magnus"},
  {4, 'UC60', "Necro'lic", "Visage", 1693, "BTNSpiritWyvern", "Visage"},
  {4, 'U00A', "Chaos Knight", "Nessaj", 1694, "BTNChaosWarlord", "Nessaj"},
  {4, 'U008', "Lycanthrope", "Banehallow", 1685, "BTNKiljaedin", "Banehallow"},
  {4, 'U006', "Broodmother", "Black Arachnia", 1677, "BTNSpiderBlack", "Broodmother"},
  {4, 'Ewar', "Phantom Assasin", "Mortred", 1633, "BTNHeroWarden", "Mortred"},

  {5, 'H00V', "Gorgon", "Medusa", 1706, "BTNNagaSeaWitch", "Medusa"},
  {5, 'Udre', "Night Stalker", "Balanar", 1636, "BTNTichondrius", "Balanar"},
  {5, 'NC00', "Skeleton King", "King Leoric", 1671, "BTNSkeletonArcher", "Leoric"},
  {5, 'UC42', "Doom Bringer", "Lucifer", 1675, "BTNDoomGuard", "Lucifer"},
  {5, 'U000', "Nerubian Assasin", "Anub'arak", 1687, "BTNHeroCryptLord", "Anubarak"},
  {5, 'UC91', "Slithereen Guard", "Slardar", 1701, "BTNNagaMyrmidon", "Slardar"},
  {5, 'UC01', "Queen of Pain", "Akasha", 1683, "BTNBlueDemoness", "Akasha"},
  {5, 'E004', "Bone Fletcher", "Clinkz", 1696, "BTNSkeletonMage", "Clinkz"},
  {5, 'EC45', "Faceless Void", "Darkterror", 1673, "BTNFacelessOne", "Void"},
  {5, 'EC77', "Netherdrake", "Viper", 1668, "BTNNetherDragon", "Viper"},
  {5, 'E002', "Lightning Revenant", "Razor", 1678, "BTNRevenant", "Razor"},
  {5, 'U00C', "Lifestealer", "N'aix", 1681, "BTNGhoul", "Naix"},

  {6, 'H00H', "Oblivion", "Pugna", 1698, "BTNPugna", "Pugna"},
  {6, 'Ofar', "Tidehunter", "Leviathan", 1626, "BTNSeaGiantGreen", "Leviathan"},
  {6, 'Oshd', "Bane Elemental", "Atropos", 1627, "BTNVoidWalker", "Atropos"},
  {6, 'U00E', "Necrolyte", "Rotund'jere", 1699, "BTNGhostOfKelThuzad", "Necrolyte"},
  {6, 'U00F', "Butcher", "Pudge", 1709, "BTNAbomination", "Pudge"},
  {6, 'O00J', "Spiritbreaker", "Barathrum", 1697, "BTNSpiritWalker", "Barathrum"},
  {6, 'Ubal', "Nerubian Weaver", "Anub'seran", 1664, "BTNNerubianQueen", "Weaver"},
  {6, 'Nfir', "Shadow Fiend", "Nevermore", 1641, "BTNShade", "Nevermore"},
  {6, 'U00K', "Sand King", "Crixalis", 1670, "BTNArachnathidGreen", "Crixalis"},
  {6, 'Opgh', "Axe", "Mogul Kahn", 1661, "BTNChaosGrom", "Axe"},
  {6, 'Hvsh', "Bloodseeker", "Strygwyr", 1656, "BTNShaman", "Strygwyr"},
  {6, 'Udea', "Lord of Avernus", "Abaddon", 1635, "BTNHeroDeathKnight", "Abaddon"},

  {7, 'E01B', "Spectre", "Mercurial", 1713, "BTNvengeanceincarnate", "Spectre"},
  {7, 'E01A', "Witch Doctor", "Vol'Jin", 1712, "BTNWitchDoctor", "Voljin"},
  {7, 'U00P', "Obsidian Destroyer", "Harbinger", 1717, "BTNDestroyer", "Harbinger"},
  {7, 'E01C', "Warlock", "Demnok Lannik", 1718, "BTNGuldan", "Warlock"},
  {7, 'H00I', "Geomancer", "Meepo", 1724, "BTNKoboldGeomancer", "Meepo"},
  {7, 'N01W', "Shadow Priest", "Dazzle", 1726, "BTNDarkTrollShadowPriest", "Dazzle"},
  {7, 'N00R', "Pit Lord", "Azgalor", 1732, "BTNPitLord", "Pitlord"},
  {7, 'H00R', "Undying", "Dirge", 1731, "BTNZombie", "Undying"},
  {7, 'H00N', "Dark Seer", "Ish'kafel", 1729, "BTNDranaiMage", "Darkseer"},
  {7, 'H00U', "Invoker", "Kael", 1729, "BTNInvoker", "Invoker"}
};

static DotaItem items[] = {
  {'0000', '0000', "Empty slot", 0, "emptyslot"},

  {'plcl', 'h028', "Clarity Potion", 50, "BTNLesserClarityPotion"},
  {'sor8', 'h029', "Flask of Sapphire Water", 100, "BTNPotionBlueBig"},
  {'silk', 'h02A', "Ancient Tango of Essifation", 90, "BTNAncientOfTheEarth"},
  {'I02J', 'h02B', "Empty Bottle", 700, "BTNBottle0"},
  {'I019', 0, "Empty Bottle", 700, "BTNBottle0"},
  {'sor7', 'h02C', "Observer Wards", 200, "BTNSentryWard"},
  {'tgrh', 'h02D', "Sentry Wards", 200, "BTNBlueSentryWard"},
  {'stwp', 'h02E', "Scroll of Town Portal", 135, "BTNScrollUber"},
  {'sman', 'h02F', "Animal Courier", 225, "BTNCritterChicken"},

  {'stre', 'h012', "Gloves of Haste", 550, "BTNGlove"},
  {'jdrn', 'h01M', "Mask of Death", 900, "BTNUndeadShrine"},
  {'tkno', 'h01X', "Ring of Regeneration", 375, "BTNRingSkull"},
  {'desc', 'h01K', "Kelen's Dagger of Escape", 2150, "BTNDaggerOfEscape"},
  {'shhn', 'h021', "Sobi Mask", 325, "BTNSobiMask"},
  {'tgxp', 'h011', "Boots of Speed", 500, "BTNBootsOfSpeed"},
  {'oflg', 'h01G', "Gem of True Sight", 750, "BTNGem"},
  {'gvsm', 'h024', "Ultimate Orb", 2100, "BTNOrbofSlowness"},
  {'sor1', 'h01R', "Planeswalker's Cloak", 650, "BTNINV_Misc_Cape_08"},

  {'tmsc', 'h018', "Blades of Attack", 650, "BTNClawsOfAttack"},
  {'sor2', 'h019', "Broadsword", 1200, "BTNSteelMelee"},
  {'whwd', 'h01U', "Quarterstaff", 900, "BTNAlleriaFlute"},
  {'sora', 'h01B', "Claymore", 1400, "BTNINV_Sword_19"},
  {'modt', 'h01W', "Ring of Protection", 175, "BTNRingGreen"},
  {'rej4', 'h023', "Stout Shield", 300, "BTNImprovedUnholyArmor"},
  {'I00Z', 'h027', "Javelin", 1400, "BTNSteelRanged"},
  {'flag', 'h01O', "Mithril Hammer", 1610, "BTNHammer"},
  {'tbsm', 'h01A', "Chainmail", 620, "BTNINV_Chest_Chain_12"},
  {'scul', 'h01H', "Helm of Iron Will", 950, "BTNHelmutPurple"},
  {'ram3', 'h01S', "Plate Mail", 1400, "BTNINV_Chest_Plate13"},

  {'stwa', 'h01F', "Gauntlets of Strength", 150, "BTNGauntletsOfOgrePower"},
  {'shdt', 'h020', "Slippers of Agility", 150, "BTNSlippersOfAgility"},
  {'sbok', 'h01L', "Mantle of Intelligence", 150, "BTNMantleOfIntelligence"},
  {'iwbr', 'h01J', "Ironwood Branch", 57, "BTNNatureTouchGrow"},
  {'bgst', 'h016', "Belt of Giant Strength", 450, "BTNBelt"},
  {'belv', 'h013', "Boots of Elvenskin", 450, "BTNBoots"},
  {'ciri', 'h01Y', "Robe of the Magi", 450, "BTNRobeOfTheMagi"},
  {'tbar', 'h015', "Circlet of Nobility", 185, "BTNCirclet"},
  {'sksh', 'h01Q', "Ogre Axe", 1000, "BTNSpiritWalkerAdeptTraining"},
  {'tmmt', 'h017', "Blade of Alacrity", 1000, "BTNINV_ThrowingKnife_03"},
  {'shtm', 'h022', "Staff of Wizardry", 1000, "BTNWandOfCyclone"},

  {'srtl', 'h01C', "Demon Edge", 2600, "BTNFrostMourne"},
  {'srbd', 'h01D', "Eaglehorn", 3300, "BTNINV_Weapon_Bow_06"},
  {'rump', 'h01N', "Messerschmidt's Reaver", 3200, "BTNSpiritWalkerMasterTraining"},
  {'shrs', 'h01Z', "Sacred Relic", 3800, "BTNStaffOfTeleportation"},
  {'sprn', 'h01I', "Hyperstone", 2100, "BTNPeriapt1"},
  {'rej6', 'h01V', "Ring of Health", 875, "BTNGoldRing"},
  {'shcw', 'h026', "Void Stone", 875, "BTNPeriapt"},
  {'ram2', 'h01P', "Mystic Staff", 2700, "BTNStaffOfNegation"},
  {'pgin', 'h01E', "Energy Booster", 1000, "BTNEnchantedGemstone"},
  {'hbth', 'h01T', "Point Booster", 1200, "BTNUsedSoulGem"},
  {'oli2', 'h025', "Vitality Booster", 1100, "BTNSoulGem"},


  {0, 0, "Perserverance", 1775, "BTNOrbOfFire"},
  {'ssil', 'h02H', "Headdress of Rejuvenation", 225, "BTNINV_Helmet_17"},
  {0, 0, "Headdress of Rejuvenation", 225, "BTNINV_Helmet_17"},
  {'rin1', 'h02I', "Nathrezim Buckler", 200, "BTNThoriumArmor"},
  {0, 0, "Nathrezim Buckler", 200, "BTNThoriumArmor"},
  {0, 0, "Ring of Basilius", 500, "BTNRingVioletSpider"},
  {'rspd', 'h02K', "Boots of Travel", 2200, "BTNAbility_Rogue_Sprint"},
  {0, 0, "Boots of Travel", 2200, "BTNAbility_Rogue_Sprint"},
  {'dsum', 'h014', "Power Treads", 350, "BTNWirtsLeg"},
  {0, 0, "Power Treads", 350, "BTNWirtsLeg"},
  {'pghe', 'h02L', "Hand of Midas", 1400, "BTNGoldGloves"},
  {0, 0, "Hand of Midas", 1400, "BTNGoldGloves"},
  {0, 0, "Oblivion Staff", 0, "BTNINV_Mace_10"},
  {'rres', 'h02N', "Bracer", 175, "BTNRunedBracers"},
  {0, 0, "Bracer", 175, "BTNRunedBracers"},
  {'sres', 'h02O', "Wraith Band", 125, "BTNRevenant"},
  {0, 0, "Wraith Band", 125, "BTNRevenant"},
  {'kpin', 'h02P', "Null Talisman", 150, "BTNTalisman"},
  {0, 0, "Null Talisman", 150, "BTNTalisman"},

  {'I003', 'h02Q', "Yasha", 800, "BTNINV_Sword_10"},
  {0, 0, "Yasha", 800, "BTNINV_Sword_10"},
  {'afac', 'h02R', "Sange", 800, "BTNJapaneseSword"},
  {0, 0, "Sange", 800, "BTNJapaneseSword"},
  {'rhe1', 'h02S', "Cranium Basher", 1460, "BTNINV_Hammer_10"},
  {0, 0, "Cranium Basher", 1460, "BTNINV_Hammer_10"},
  {'rwat', 'h02T', "Blade Mail", 500, "BTNINV_Chest_Plate06"},
  {0, 0, "Blade Mail", 500, "BTNINV_Chest_Plate06"},
  {'moon', 'h02U', "Maelstrom", 1300, "BTNStormHammer"},
  {0, 0, "Maelstrom", 1300, "BTNStormHammer"},
  {'manh', 'h02V', "Diffusal Blade", 1550, "BTNINV_Sword_11"},
  {0, 0, "Diffusal Blade", 1550, "BTNINV_Sword_11"},
  {0, 0, "Helm of Dominator", 1850, "BTNINV_Helmet_13"},
  {'pmna', 'h02X', "Mask of Madness", 1050, "BTNHelmOfValor"},
  {0, 0, "Mask of Madness", 1050, "BTNHelmOfValor"},
  {'gfor', 'h02Y', "Eul's Scepter of Divinity", 450, "BTNStaffofpurification"},
  {0, 0, "Eul's Scepter of Divinity", 450, "BTNStaffofpurification"},
  {0, 0, "Soul Booster", 0, "BTNPhilosophersStone"},
  {'shar', 'h030', "Mekansm", 900, "BTNSpellShieldAmulet"},
  {0, 0, "Mekansm", 900, "BTNSpellShieldAmulet"},

  {'wlsd', 'h031', "Sange and Yasha", 800, "BTNSpell_Holy_BlessingOfStrength"},
  {0, 0, "Sange and Yasha", 800, "BTNSpell_Holy_BlessingOfStrength"},
  {'totw', 'h032', "Stygian Desolator", 1200, "BTNAbility_Gouge"},
  {0, 0, "Stygian Desolator", 1200, "BTNAbility_Gouge"},
  {0, 0, "Battle Fury", 0, "BTNINV_ThrowingAxe_06"},
  {'rre2', 'h034', "Crystalis", 500, "BTNThoriumMelee"},
  {0, 0, "Crystalis", 500, "BTNThoriumMelee"},
  {'tdx2', 'h035', "Black King Bar", 1600, "BTNRodOfNecromancy"},
  {0, 0, "Black King Bar", 1600, "BTNRodOfNecromancy"},
  {'rde1', 'h036', "Manta Style", 1400, "BTNINV_ThrowingAxe_02"},
  {0, 0, "Manta Style", 1400, "BTNINV_ThrowingAxe_02"},
  {'tst2', 0, "Aegis of the Immortal", 2000, "BTNArcaniteArmor"},
  {0, 0, "Aegis of the Immortal", 2000, "BTNArcaniteArmor"},
  {'rde3', 'h037', "Lothar's Edge", 650, "BTNLothars"},
  {0, 0, "Lothar's Edge", 1400, "BTNLothars"},
  {'rhe3', 'h038', "Dagon", 1350, "BTNINV_Wand_06"},
  {0, 0, "Dagon 1", 1350, "BTNINV_Wand_06"},
  {0, 0, "Dagon 2", 1350, "BTNINV_Wand_06"},
  {0, 0, "Dagon 3", 1350, "BTNINV_Wand_06"},
  {0, 0, "Dagon 4", 1350, "BTNINV_Wand_06"},
  {0, 0, "Dagon 5", 1350, "BTNINV_Wand_06"},
  {'ajen', 'h039', "Necronomicon", 1300, "BTNNecromancerMaster"},
  {0, 0, "Necronomicon 1", 1300, "BTNNecromancerMaster"},
  {0, 0, "Necronomicon 2", 1300, "BTNNecromancerMaster"},
  {0, 0, "Necronomicon 3", 1300, "BTNNecromancerMaster"},
  {'hslv', 'h03A', "Linken's Sphere", 1325, "BTNOrbofWater"},
  {0, 0, "Linken's Sphere", 1325, "BTNOrbofWater"},

  {0, 0, "Divine Rapier", 0, "BTNINV_Sword_25"},
  {'rman', 'h03D', "Buriza-do Kyanon", 1000, "BTNINV_Weapon_Crossbow_10"},
  {0, 0, "Buriza-do Kyanon", 1250, "BTNINV_Weapon_Crossbow_10"},
  {'odef', 0, "Monkey King Bar", 1650, "BTNINV_Weapon_Halberd_10"},
  {0, 0, "Monkey King Bar", 1650, "BTNINV_Weapon_Halberd_10"},
  {'clsd', 'h03F', "Radiance", 1525, "BTNTransmute"},
  {0, 0, "Radiance", 1525, "BTNTransmute"},
  {'rag1', 'h03G', "Heart of Tarrasque", 1200, "BTNHeartOfAszune"},
  {0, 0, "Heart of Tarrasque", 1200, "BTNHeartOfAszune"},
  {'tsct', 'h03H', "Satanic", 1100, "BTNHornOfDoom"},
  {0, 0, "Satanic", 1100, "BTNHornOfDoom"},
  {'lmbr', 'h03I', "Eye of Skadi", 1250, "BTNIceShard"},
  {0, 0, "Eye of Skadi", 1250, "BTNIceShard"},
  {'fgfh', 'h03J', "The Butterfly", 1800, "BTNINV_ThrowingKnife_04"},
  {0, 0, "The Butterfly", 1800, "BTNINV_ThrowingKnife_04"},
  {0, 0, "Aghanim's Scepter", 0, "BTNINV_Wand_05"},
  {'rat9', 'h03L', "Refresher Orb", 1875, "BTNHeartOfSearinox"},
  {0, 0, "Refresher Orb", 1875, "BTNHeartOfSearinox"},
  {'evtl', 0, "Guinsoo's Scythe of Vyse", 450, "BTNINV_Sword_09"},
  {0, 0, "Guinsoo's Scythe of Vyse", 450, "BTNINV_Sword_09"},

  {0, 0, "Vanguard", 0, "BTNAdvancedUnholyArmor"},
  {'I00R', 'h03O', "Arcane Ring", 525, "BTNArcaneRing"},
  {0, 0, "Arcane Ring", 525, "BTNArcaneRing"},
  {0, 0, "Mjollnir", 0, "BTNIceHammer"},
  {'I00X', 'h03Q', "Flying Courier", 200, "BTNRavenForm"},
  {0, 0, "Flying Courier", 200, "BTNRavenForm"},
  {0, 0, "Flying Courier with mana", 200, "BTNRavenForm"},
  {'I013', 'h03R', "Vladmir's Offering", 300, "BTNCloakOfFlames"},
  {0, 0, "Vladmir's Offering", 625, "BTNCloakOfFlames"},
  {'I01L', 'h03S', "Assault Cuirass", 2000, "BTNINV_Chest_Chain_14"},
  {0, 0, "Assault Cuirass", 2000, "BTNINV_Chest_Chain_14"},
  {0, 0, "Bloodstone", 0, "BTNINV_Misc_Gem_Bloodstone_02"},
  {0, 0, "Hood of Defiance", 0, "BTNHoodOfCunning"},
  {'I01R', 'h03V', "Armlet of Mordiggian", 700, "BTNImprovedUnholyStrength"},
  {0, 0, "Armlet of Mordiggian", 700, "BTNImprovedUnholyStrength"},
  {'I01T', 'h03W', "Shiva's Guard", 600, "BTNAdvancedMoonArmor"},
  {0, 0, "Shiva's Guard", 600, "BTNAdvancedMoonArmor"},
  {'I00A', 'h03X', "Orchid Malevolence", 1000, "BTNStaffOfSilence"},
  {0, 0, "Orchid Malevolence", 1000, "BTNStaffOfSilence"}
};
static DotaItem itemBackup[sizeof items / sizeof items[0]];

static DotaAbility abilities[] = {
  {'0000', "None", 0, 5, "Empty"},
  {'A02A', "Magic Missile", 'Hvwd', 0, "BTNFrostBolt"},
  {'A0AP', "Terror", 'Hvwd', 1, "BTNHowlOfTerror"},
  {'A045', "Command Aura", 'Hvwd', 2, "PASBTNGnollCommandAura"},
  {'A00G', "Nether Swap", 'Hvwd', 3, "BTNWandOfNeutralization"},
  {'A0IN', "Nether Swap", 'Hvwd', 3, "BTNWandOfNeutralization"},
  {'A020', "Arc Lightning", 'Hmbr', 0, "BTNChainLightning"},
  {'A0JC', "Lightning Bolt", 'Hmbr', 1, "BTNBlue_Lightning"},
  {'A0N5', "Static Field", 'Hmbr', 2, "PASBTNZeusStatic"},
  {'A07C', "Thundergod's Wrath", 'Hmbr', 3, "BTNSpell_Holy_SealOfMight"},
  {'A06L', "Thundergod's Wrath", 'Hmbr', 3, "BTNSpell_Holy_SealOfMight"},
  {'A0DY', "Impetus", 'Emoo', 0, "BTNimpalingbolt"},
  {'A0DX', "Enchant", 'Emoo', 1, "BTNDispelMagic"},
  {'A01B', "Nature's Attendants", 'Emoo', 2, "BTNRejuvenation"},
  {'A0DW', "Untouchable", 'Emoo', 3, "PASBTNMagicImmunity"},
  {'A0FN', "Waveform", 'O00P', 0, "BTNCrushingWave"},
  {'A0G6', "Adaptive Strike", 'O00P', 1, "BTNShimmerWeed"},
  {'A0KX', "Morph", 'O00P', 2, "BTNReplenishManaOn"},
  {'A0G8', "Replicate", 'O00P', 3, "BTNForceofNature"},
  {'A01D', "Frost Nova", 'Hjai', 0, "BTNGlacier"},
  {'A04C', "Frostbite", 'Hjai', 1, "BTNFrost"},
  {'AHab', "Brilliance Aura", 'Hjai', 2, "PASBTNBrilliance"},
  {'A03R', "Freezing Field", 'Hjai', 3, "BTN_CMFF"},
  {'A0AV', "Freezing Field", 'Hjai', 3, "BTN_CMFF"},
  {'A0RZ', "Storm Bolt", 'H001', 0, "BTNStormBolt"},
  {'AHtb', "Storm Bolt", 'H001', 0, "BTNStormBolt"},
  {'A01K', "Great Cleave", 'H001', 1, "PASBTNCleavingAttack"},
  {'A01M', "Toughness Aura", 'H001', 2, "PASBTNDevotion"},
  {'A01H', "God's Strength", 'H001', 3, "BTNSvenGS"},
  {'A0WP', "God's Strength", 'H001', 3, "BTNSvenGS"},
  {'A063', "Mirror Image", 'HC49', 0, "BTNSirenMirrorImage"},
  {'A0BA', "Ensnare", 'HC49', 1, "BTNEnsnare"},
  {'A00E', "Critical Strike", 'HC49', 2, "PASBTNCriticalStrike"},
  {'A07U', "Song of the Siren", 'HC49', 3, "BTNPenguin"},
  {'A0SK', "Fissure", 'Otch', 0, "BTNShockWave"},
  {'A0M0', "Fissure", 'Otch', 0, "BTNShockWave"},
  {'A0DL', "Enchant Totem", 'Otch', 1, "BTNSmash"},
  {'A0DJ', "Aftershock", 'Otch', 2, "PASBTNAftershock"},
  {'A0DH', "Echo Slam", 'Otch', 3, "BTNEarthquake"},
  {'A0RG', "Smoke Screen", 'HC92', 0, "BTNCloudOfFog"},
  {'A0K9', "Blink Strike", 'HC92', 1, "BTNBearBlink"},
  {'A0DZ', "Backstab", 'HC92', 2, "BTNTheBlackArrow"},
  {'A00J', "Permanent Invisibility", 'HC92', 3, "BTNRikiINV"},
  {'A0A5', "Spirit Bear", 'N01O', 0, "BTNGrizzlyBear"},
  {'A0AA', "Rabid", 'N01O', 1, "BTNDOCMasterTraining"},
  {'A0AB', "Rabid", 'N01O', 1, "BTNDOCMasterTraining"},
  {'A0AC', "Rabid", 'N01O', 1, "BTNDOCMasterTraining"},
  {'A0AD', "Rabid", 'N01O', 1, "BTNDOCMasterTraining"},
  {'A0AE', "Rabid", 'N01O', 1, "BTNDOCMasterTraining"},
  {'A0A8', "Synergy", 'N01O', 2, "PASBTN_SyllaSynergy"},
  {'A0AG', "True Form", 'N01O', 3, "BTNBearForm"},
  {'A01F', "Dragon Slave", 'H004', 0, "BTNSearingArrows"},
  {'A027', "Light Strike Array", 'H004', 1, "BTN_LightStrikeArray"},
  {'A001', "Ultimate", 'H004', 2, "BTNDisenchant"},
  {'A01P', "Laguna Blade", 'H004', 3, "BTNManaFlare"},
  {'A09Z', "Laguna Blade", 'H004', 3, "BTNManaFlare"},
  {'A05G', "Blade Fury", 'Nbbc', 0, "BTNWhirlwind"},
  {'A047', "Healing Ward", 'Nbbc', 1, "BTNHealingWard"},
  {'A00K', "Blade Dance", 'Nbbc', 2, "BTNAbility_Parry"},
  {'A0M1', "Omnislash", 'Nbbc', 3, "BTNOmnislash"},
  {'A0KD', "Curse of the Silent", 'N01A', 0, "BTNControlMagic"},
  {'A0LZ', "Glaives of Wisdom", 'N01A', 1, "BTN_GlaviesOfWisdom"},
  {'A0LR', "Last Word", 'N01A', 2, "BTNSell"},
  {'A0L3', "Global Silence", 'N01A', 3, "BTNSilencerSilence"},
  {'A01Z', "Nature's Guise", 'Hamg', 0, "BTNAmbush"},
  {'A01V', "Eyes in the Forest", 'Hamg', 1, "BTNUltravision"},
  {'A01U', "Living Armor", 'Hamg', 2, "BTNNaturesBlessing"},
  {'A07Z', "Overgrowth", 'Hamg', 3, "BTNSpikedBarricades"},
  {'A0I7', "Malefice", 'Uktl', 0, "BTNvengeanceincarnate"},
  {'A0B7', "Conversion", 'Uktl', 1, "BTN_Conversion"},
  {'A0B1', "Midnight Pulse", 'Uktl', 2, "BTNDeathAndDecay"},
  {'A0BY', "Black Hole", 'Uktl', 3, "BTNGenericSpellImmunity"},
  {'A085', "Illuminate", 'Hblm', 0, "BTNOrbOfLightning"},
  {'A07Y', "Mana Leak", 'Hblm', 1, "BTNCharm"},
  {'A07N', "Chakra Magic", 'Hblm', 2, "BTNPriestAdept"},
  {'A0MO', "Ignis Fatuus", 'Hblm', 3, "BTNWispSplode"},
  {'A0EO', "Ignis Fatuus", 'Hblm', 3, "BTNWispSplode"},
  {'A03Y', "Earthshock", 'Huth', 0, "BTNEarthquake"},
  {'A059', "Overpower", 'Huth', 1, "BTNBloodLustOn"},
  {'ANic', "Fury Swipes", 'Huth', 2, "BTNAbility_Druid_Rake"},
  {'A0LC', "Enrage", 'Huth', 3, "BTNImprovedStrengthOfTheWild"},
  {'A04W', "Fireblast", 'Hmkg', 0, "BTNFireBolt"},
  {'A089', "Fireblast", 'Hmkg', 0, "BTNFireBolt"},
  {'A08A', "Fireblast", 'Hmkg', 0, "BTNFireBolt"},
  {'A08D', "Fireblast", 'Hmkg', 0, "BTNFireBolt"},
  {'A011', "Ignite", 'Hmkg', 1, "BTNLiquidFire"},
  {'A007', "Ignite", 'Hmkg', 1, "BTNLiquidFire"},
  {'A01T', "Ignite", 'Hmkg', 1, "BTNLiquidFire"},
  {'A00F', "Ignite", 'Hmkg', 1, "BTNLiquidFire"},
  {'A083', "Bloodlust", 'Hmkg', 2, "BTNBloodLustOn"},
  {'A08F', "Bloodlust", 'Hmkg', 2, "BTNBloodLustOn"},
  {'A08G', "Bloodlust", 'Hmkg', 2, "BTNBloodLustOn"},
  {'A08I', "Bloodlust", 'Hmkg', 2, "BTNBloodLustOn"},
  {'A088', "Multi Cast", 'Hmkg', 3, "BTNWitchDoctorMaster"},
  {'A049', "Laser", 'Ntin', 0, "BTNTinkerLaser"},
  {'A05E', "Heat Seeking Missile", 'Ntin', 1, "BTNClusterRockets"},
  {'A0BQ', "March of the Machines", 'Ntin', 2, "BTNClockWerkGoblin"},
  {'A065', "Rearm", 'Ntin', 3, "BTNEngineeringUpgrade"},
  {'A06Q', "Sprout", 'Emns', 0, "BTNEatTree"},
  {'A01O', "Teleportation", 'Emns', 1, "BTNWispSplode"},
  {'AEfn', "Force of Nature", 'Emns', 2, "BTNEnt"},
  {'A07X', "Wrath of Nature", 'Emns', 3, "BTNTreeOfEternity"},
  {'A0AL', "Wrath of Nature", 'Emns', 3, "BTNTreeOfEternity"},
  {'A0DA', "Spirit Lance", 'Ogrh', 0, "BTNWindsArrows"},
  {'A0D7', "Doppelwalk", 'Ogrh', 1, "BTNDoppelwalk"},
  {'A0DB', "Juxtapose", 'Ogrh', 2, "BTNMassTeleport"},
  {'A0D9', "Phantom Edge", 'Ogrh', 3, "BTNBanish"},
  {'A0LL', "Avalanche", 'Ucrl', 0, "BTNGolemStormBolt"},
  {'A0BZ', "Toss", 'Ucrl', 1, "BTNattackground"},
  {'A0BU', "Craggy Exterior", 'Ucrl', 2, "PASBTNResistantSkin"},
  {'A0CY', "Grow", 'Ucrl', 3, "PASBTNTinyGrow"},
  {'A05J', "Land Mines", 'H00K', 0, "BTNGoblinLandMine"},
  {'A06H', "Stasis Trap", 'H00K', 1, "BTNStasisTrap"},
  {'A06B', "Suicide Squad, Attack!", 'H00K', 2, "BTNSelfDestructOn"},
  {'A0AK', "Remote Mines", 'H00K', 3, "BTNRemote"},
  {'A0KM', "Penitence", 'H00A', 0, "BTNResurrection"},
  {'A0LV', "Test of Faith", 'H00A', 1, "BTNStaffOfPreservation"},
  {'A069', "Holy Persuasion", 'H00A', 2, "BTNScepterOfMastery"},
  {'A0LT', "Hand of God", 'H00A', 3, "BTNHeal"},
  {'A042', "Lucent Beam", 'E005', 0, "BTNMoonStone"},
  {'A041', "Moon Glaive", 'E005', 1, "PASBTNUpgradeMoonGlaive"},
  {'A062', "Lunar Blessing", 'E005', 2, "BTNAdvancedStrengthOfTheMoon"},
  {'A054', "Eclipse", 'E005', 3, "BTNElunesBlessing"},
  {'A00U', "Eclipse", 'E005', 3, "BTNElunesBlessing"},
  {'A064', "Scatter Shot", 'Usyl', 0, "BTNFragmentationBombs"},
  {'A03S', "Headshot", 'Usyl', 1, "BTNHumanMissileUpOne"},
  {'A03U', "Take Aim", 'Usyl', 2, "BTNHumanMissileUpTwo"},
  {'A04P', "Assassinate", 'Usyl', 3, "BTNDwarvenLongRifle"},
  {'A0BE', "Berserker Rage", 'N016', 0, "BTNOrcMeleeUpThree"},
  {'A0BC', "Blind", 'N016', 1, "BTNSleep"},
  {'A0BD', "Fervor", 'N016', 2, "PASBTNCommand"},
  {'A0BB', "Rampage", 'N016', 3, "BTNBerserkForTrolls"},
  {'A010', "Forked Lightning", 'Orkn', 0, "BTNRhastaChain"},
  {'A0RX', "Voodoo", 'Orkn', 1, "BTNHex"},
  {'A0MN', "Voodoo", 'Orkn', 1, "BTNHex"},
  {'A00P', "Shackles", 'Orkn', 2, "BTNMagicLariet"},
  {'A00H', "Mass Serpent Ward", 'Orkn', 3, "BTNSerpentWard"},
  {'A0A1', "Mass Serpent Ward", 'Orkn', 3, "BTNSerpentWard"},
  {'A0FW', "Viscous Nasal Goo", 'H008', 0, "BTNBristleGoo"},
  {'A0GP', "Quill Spray", 'H008', 1, "BTNFanOfKnives"},
  {'A0M3', "Bristleback", 'H008', 2, "BTNQuillSprayOff"},
  {'A0FV', "Warpath", 'H008', 3, "BTNHire"},
  {'A06M', "Thunder Clap", 'Npbm', 0, "BTNThunderclap"},
  {'Acdh', "Drunken Haze", 'Npbm', 1, "BTNStrongDrink"},
  {'A0MX', "Drunken Brawler", 'Npbm', 2, "PASBTNDrunkenDodge"},
  {'A0MQ', "Primal Split", 'Npbm', 3, "BTNStormEarthFire"},
  {'A00S', "Hoof Stomp", 'H000', 0, "BTNWarStomp"},
  {'A00L', "Double Edge", 'H000', 1, "BTNArcaniteMelee"},
  {'A00V', "Return", 'H000', 2, "PASBTNThorns"},
  {'A01L', "Great Fortitude", 'H000', 3, "BTNcentaur"},
  {'A004', "Shuriken Toss", 'Naka', 0, "BTNUpgradeMoonGlaive"},
  {'A000', "Jinada", 'Naka', 1, "PASBTNEvasion"},
  {'A07A', "Wind Walk", 'Naka', 2, "BTNWindWalkOn"},
  {'A0B4', "Track", 'Naka', 3, "BTNSpy"},
  {'A03F', "Breathe Fire", 'Hlgr', 0, "BTNBreathOfFire"},
  {'A0AR', "Dragon Tail", 'Hlgr', 1, "BTNHumanArmorUpThree"},
  {'A0CL', "Dragon Blood", 'Hlgr', 2, "PASBTNIncinerate"},
  {'A03G', "Elder Dragon Form", 'Hlgr', 3, "BTNAzureDragon"},
  {'A022', "Mana Break", 'Edem', 0, "PASBTNFeedBack"},
  {'AEbl', "Blink", 'Edem', 1, "BTNBlink"},
  {'A0KY', "Spell Shield", 'Edem', 2, "BTNNeutralManaShield"},
  {'A0E3', "Mana Void", 'Edem', 3, "BTNTelekinesis"},
  {'A026', "Frost Arrows", 'Nbrn', 0, "BTNColdArrowsOn"},
  {'A0QB', "Silence", 'Nbrn', 1, "BTNDrowSilence"},
  {'ANsi', "Silence", 'Nbrn', 1, "BTNDrowSilence"},
  {'A029', "Trueshot Aura", 'Nbrn', 2, "PASBTNTrueShot"},
  {'A056', "Marksmanship", 'Nbrn', 3, "BTNMarksmanship"},
  {'A0VC', "Marksmanship", 'Nbrn', 3, "BTNMarksmanship"},
  {'A08N', "Purification", 'Harf', 0, "BTNHolyBolt"},
  {'A08V', "Repel", 'Harf', 1, "BTNMagicalSentry"},
  {'A06A', "Degen Aura", 'Harf', 2, "BTNSpiritLink"},
  {'A0ER', "Guardian Angel", 'Harf', 3, "BTNDivineIntervention"},
  {'A0O1', "Wild Axes", 'H00D', 0, "BTNAbility_Warrior_SavageBlow"},
  {'A0OO', "Call of the Wild", 'H00D', 1, "BTNEnchantedCrows"},
  {'A0O0', "Beast Rage", 'H00D', 2, "BTNEnchantedBears"},
  {'A0O2', "Primal Roar", 'H00D', 3, "BTNBattleRoar"},
  {'A0O7', "Dual Breath", 'E00P', 0, "BTNDualBreath"},
  {'A0O6', "Ice Path", 'E00P', 1, "BTNFreezingBreath"},
  {'A0O8', "Auto Fire", 'E00P', 2, "PASBTNAutoFire"},
  {'A0O5', "Macropyre", 'E00P', 3, "BTNBreathOfFireNew"},
  {'A0IL', "Acid Spray", 'N01I', 0, "BTNHealingSpray"},
  {'A0J6', "Unstable Concoction", 'N01I', 1, "BTNUnstableConcoction"},
  {'A0O3', "Goblin's Greed", 'N01I', 2, "BTNMGexchange"},
  {'ANcr', "Chemical Rage", 'N01I', 3, "BTNChemicalRage"},
  {'A0KV', "Starfall", 'N01V', 0, "BTNStarfall"},
  {'A0L8', "Elune's Arrow", 'N01V', 1, "BTNImprovedStrengthOfTheMoon"},
  {'A0LN', "Leap", 'N01V', 2, "BTNAdvancedStrengthOfTheWild"},
  {'A0KU', "Moonlight Shadow", 'N01V', 3, "BTNAmbush"},
  {'A0QY', "Electric Rave", 'H00S', 0, "BTNAbility_Druid_CatForm"},
  {'A0R6', "Barrier", 'H00S', 1, "BTNManaRecharge"},
  {'A0QW', "Overload", 'H00S', 2, "PASBTNOverload"},
  {'A0R1', "Lightning Grapple", 'H00S', 3, "BTNStormReach"},
  {'A0QP', "Inner Vitality", 'H00Q', 0, "BTNRegenerate"},
  {'A0QN', "Burning Spear", 'H00Q', 1, "BTNAutoFlamingArrows"},
  {'A0QQ', "Berserker's Blood", 'H00Q', 2, "PASBTNBerserkerBlood"},
  {'A0QR', "Life Break", 'H00Q', 3, "BTNSoulEater"},
  {'A0RE', "Refraction", 'E01Y', 0, "BTNRefraction"},
  {'A0RV', "Meld", 'E01Y', 1, "BTNWardenHide"},
  {'A0RO', "Psi Blades", 'E01Y', 2, "PASBTNVorpal"},
  {'A0RP', "Psionic Trap", 'E01Y', 3, "BTNTrap"},
  {'A0S9', "Illusory Orb", 'N00B', 0, "BTNIllusory"},
  {'A0SC', "Waning Rift", 'N00B', 1, "BTNWanningRift"},
  {'A0SB', "Phase Shift", 'N00B', 2, "BTNPhaseShiftOn"},
  {'A0S8', "Dream Coil", 'N00B', 3, "BTNManaFlare"},
  {'A04L', "Soul Steal", 'Eevi', 0, "BTNDrainSoul"},
  {'A0H4', "Conjure Image", 'Eevi', 1, "BTNConjure"},
  {'A0MV', "Metamorphosis", 'Eevi', 2, "BTNMetamorphosis"},
  {'A07Q', "Sunder", 'Eevi', 3, "BTNDeathCoil"},
  {'A06W', "Split Earth", 'Ekee', 0, "BTNEarthquake"},
  {'A035', "Diabolic Edict", 'Ekee', 1, "BTNGuldanSkull"},
  {'A06V', "Lightning Storm", 'Ekee', 2, "BTNLeshLightStorm"},
  {'A06X', "Pulse Nova", 'Ekee', 3, "PASBTNShadeTrueSight"},
  {'A0AQ', "Pulse Nova", 'Ekee', 3, "PASBTNShadeTrueSight"},
  {'A07F', "Frost Nova", 'Ulic', 0, "BTNGlacier"},
  {'A08R', "Frost Armor", 'Ulic', 1, "BTNFrostArmorOn"},
  {'A053', "Dark Ritual", 'Ulic', 2, "BTNDarkRitual"},
  {'A05T', "Chain Frost", 'Ulic', 3, "BTNBreathOfFrost"},
  {'A08H', "Chain Frost", 'Ulic', 3, "BTNBreathOfFrost"},
  {'A02M', "Carrion Swarm", 'UC76', 0, "BTNCarrionSwarm"},
  {'A06N', "Carrion Swarm", 'UC76', 0, "BTNCarrionSwarm"},
  {'A072', "Carrion Swarm", 'UC76', 0, "BTNCarrionSwarm"},
  {'A074', "Carrion Swarm", 'UC76', 0, "BTNCarrionSwarm"},
  {'A078', "Carrion Swarm", 'UC76', 0, "BTNCarrionSwarm"},
  {'A0P6', "Silence", 'UC76', 1, "BTNSilence"},
  {'A07H', "Silence", 'UC76', 1, "BTNSilence"},
  {'A07I', "Silence", 'UC76', 1, "BTNSilence"},
  {'A07J', "Silence", 'UC76', 1, "BTNSilence"},
  {'A07M', "Silence", 'UC76', 1, "BTNSilence"},
  {'A02C', "Witchcraft", 'UC76', 2, "PASBTNKrobWitchcraft"},
  {'A073', "Exorcism", 'UC76', 3, "BTNDevourMagic"},
  {'A03J', "Exorcism", 'UC76', 3, "BTNDevourMagic"},
  {'A04J', "Exorcism", 'UC76', 3, "BTNDevourMagic"},
  {'A04M', "Exorcism", 'UC76', 3, "BTNDevourMagic"},
  {'A04N', "Exorcism", 'UC76', 3, "BTNDevourMagic"},
  {'A02J', "Impale", 'UC18', 0, "BTNImpale"},
  {'A0X5', "Impale", 'UC18', 0, "BTNImpale"},
  {'A0MN', "Voodoo", 'UC18', 1, "BTNHex"},
  {'A02N', "Mana Drain", 'UC18', 2, "BTNManaDrain"},
  {'A095', "Finger of Death", 'UC18', 3, "BTNCorpseExplode"},
  {'A09W', "Finger of Death", 'UC18', 3, "BTNCorpseExplode"},
  {'AEsh', "Shadow Strike", 'EC57', 0, "BTNShadowStrike"},
  {'A0MY', "Poison Sting", 'EC57', 1, "PASBTNEnvenomedSpear"},
  {'A0MS', "Plague Ward", 'EC57', 2, "BTNVenomWards"},
  {'A013', "Poison Nova", 'EC57', 3, "BTNCorrosiveBreath"},
  {'A0A6', "Poison Nova", 'EC57', 3, "BTNCorrosiveBreath"},
  {'A02S', "Shockwave", 'UC11', 0, "BTNShockWave"},
  {'A037', "Empower", 'UC11', 1, "BTNGhoulFrenzy"},
  {'A024', "Mighty Swing", 'UC11', 2, "BTNAbility_Warrior_Cleave"},
  {'A06F', "Reverse Polarity", 'UC11', 3, "BTNGolemThunderclap"},
  {'A08X', "Grave Chill", 'UC60', 0, "BTNAbsorbMagic"},
  {'A0C4', "Soul Assumption", 'UC60', 1, "BTNCloakOfFlames"},
  {'A0VY', "Soul Assumption", 'UC60', 1, "BTNCloakOfFlames"},
  {'A0MD', "Gravekeeper's Cloak", 'UC60', 2, "BTNAntiMagicShell"},
  {'A0VX', "Gravekeeper's Cloak", 'UC60', 2, "BTNAntiMagicShell"},
  {'A07K', "Raise Revenants", 'UC60', 3, "BTNGargoyle"},
  {'A055', "Chaos Bolt", 'U00A', 0, "BTNStun"},
  {'A0RW', "Blink Strike", 'U00A', 1, "BTNBearBlink"},
  {'A03N', "Critical Strike", 'U00A', 2, "BTNCriticalStrikeNew"},
  {'A03O', "Phantasm", 'U00A', 3, "BTNMirrorImage"},
  {'A03D', "Summon Wolves", 'U008', 0, "BTNSpiritWolf"},
  {'A02G', "Howl", 'U008', 1, "BTNHowlOfTerror"},
  {'A03E', "Feral Heart", 'U008', 2, "BTNHeartOfAszune"},
  {'A093', "Shapeshift", 'U008', 3, "BTNDireWolf"},
  {'A0BH', "Spawn Spiderlings", 'U006', 0, "BTNSpiderGreen"},
  {'A0BG', "Spin Web", 'U006', 1, "BTNWeb"},
  {'A0BK', "Incapacitating Bite", 'U006', 2, "BTNRedDragonDevour"},
  {'A0BP', "Insatiable Hunger", 'U006', 3, "BTNspiderling"},
  {'AEsh', "Shadow Strike", 'Ewar', 0, "BTNShadowStrike"},
  {'A0YM', "Stifling Dagger", 'Ewar', 0, "BTNThoriumRanged"},
  {'A0PL', "Blink Strike", 'Ewar', 1, "BTNBearBlink"},
  {'A03P', "Blur", 'Ewar', 2, "PASBTNPABlur"},
  {'A03Q', "Coup de Grace", 'Ewar', 3, "BTNAbility_BackStab"},
  {'A012', "Split Shot", 'H00V', 0, "BTNMultyArrows"},
  {'A0G2', "Chain Lightning", 'H00V', 1, "BTNChainLightning"},
  {'A0MP', "Mana Shield", 'H00V', 2, "BTNNeutralManaShield"},
  {'A02V', "Purge", 'H00V', 3, "BTNPurge"},
  {'A02H', "Void", 'Udre', 0, "BTNDarkSummoning"},
  {'A08E', "Crippling Fear", 'Udre', 1, "BTNPossession"},
  {'A086', "Hunter in the Night", 'Udre', 2, "BTNHeroDreadLord"},
  {'A03K', "Darkness", 'Udre', 3, "BTNOrbOfDarkness"},
  {'AHtb', "Storm Bolt", 'NC00', 0, "BTNStormBolt"},
  {'AUav', "Vampiric Aura", 'NC00', 1, "PASBTNVampiricAura"},
  {'AOcr', "Critical Strike", 'NC00', 2, "PASBTNCriticalStrike"},
  {'A01Y', "Reincarnation", 'NC00', 3, "PASBTNRe"},
  {'A05Y', "Devour", 'UC42', 0, "BTNRedDragonDevour"},
  {'A0FE', "Scorched Earth", 'UC42', 1, "BTNWallOfFire"},
  {'A094', "LVL? Death", 'UC42', 2, "BTNGuldanSkull"},
  {'A0MU', "Doom", 'UC42', 3, "BTNDoom"},
  {'A0A2', "Doom", 'UC42', 3, "BTNDoom"},
  {'A09K', "Impale", 'U000', 0, "BTNImpale"},
  {'A0X7', "Impale", 'U000', 0, "BTNImpale"},
  {'A02K', "Mana Burn", 'U000', 1, "BTNManaBurn"},
  {'A02L', "Spiked Carapace", 'U000', 2, "PASBTNThornShield"},
  {'A09U', "Vendetta", 'U000', 3, "BTNWindWalkOn"},
  {'A05C', "Sprint", 'UC91', 0, "BTNSirenMaster"},
  {'A01W', "Slithereen Crush", 'UC91', 1, "BTNHydraWarStomp"},
  {'A0JJ', "Bash", 'UC91', 2, "PASBTNBash"},
  {'A034', "Amplify Damage", 'UC91', 3, "BTNFaerieFireOn"},
  {'A0Q7', "Shadow Strike", 'UC01', 0, "BTNShadowStrike"},
  {'A0ME', "Blink", 'UC01', 1, "BTNBlink"},
  {'A04A', "Scream of Pain", 'UC01', 2, "BTNPossession"},
  {'A00O', "Sonic Wave", 'UC01', 3, "BTNTornado"},
  {'A0AF', "Sonic Wave", 'UC01', 3, "BTNTornado"},
  {'A030', "Strafe", 'E004', 0, "BTNSkeletalLongevity"},
  {'AHfa', "Searing Arrows", 'E004', 1, "BTNSearingArrowsOn"},
  {'A025', "Wind Walk", 'E004', 2, "BTNWindWalkOn"},
  {'A04Q', "Death Pact", 'E004', 3, "BTNDeathPact"},
  {'A0LK', "Time Walk", 'EC45', 0, "BTNInvisibility"},
  {'A0CZ', "Backtrack", 'EC45', 1, "BTNBackTrack"},
  {'A081', "Time Lock", 'EC45', 2, "PASBTNBash"},
  {'A0J1', "Chronosphere", 'EC45', 3, "BTNSeaOrb"},
  {'A05D', "Frenzy", 'EC77', 0, "BTNUnholyFrenzy"},
  {'A09V', "Poison Attack", 'EC77', 1, "BTNSlowPoison"},
  {'A0MM', "Corrosive Skin", 'EC77', 2, "PASBTNMagicImmunity"},
  {'A080', "Viper Strike", 'EC77', 3, "BTNShadowStrike"},
  {'A0RY', "Frenzy", 'E002', 0, "BTNUnholyFrenzy"},
  {'A00Y', "Chain Lightning", 'E002', 1, "BTNRazorChain"},
  {'A00N', "Unholy Fervor", 'E002', 2, "PASBTNUnholyAura"},
  {'A04B', "Storm Seeker", 'E002', 3, "BTNMonsoon"},
  {'A0T2', "Rage", 'U00C', 0, "BTNUnholyFrenzy"},
  {'A0SS', "Feast", 'U00C', 1, "BTNCannibalize"},
  {'A0TI', "Open Wounds", 'U00C', 2, "BTNGhoulFrenzy"},
  {'A0SW', "Infest", 'U00C', 3, "BTNOrbOfCorruption"},
  {'A0MT', "Nether Blast", 'H00H', 0, "BTNOrbOfLightning"},
  {'A0CE', "Decrepify", 'H00H', 1, "BTNCripple"},
  {'A09D', "Nether Ward", 'H00H', 2, "BTNEntrapmentWard"},
  {'A0CC', "Life Drain", 'H00H', 3, "BTNLifeDrain"},
  {'A02Z', "Life Drain", 'H00H', 3, "BTNLifeDrain"},
  {'A046', "Gush", 'Ofar', 0, "BTNSummonWaterElemental"},
  {'A04E', "Kraken Shell", 'Ofar', 1, "BTNNagaArmorUp3"},
  {'A044', "Anchor Smash", 'Ofar', 2, "BTNSeaGiantCriticalStrike"},
  {'A03Z', "Ravage", 'Ofar', 3, "BTNSeaGiantWarStomp"},
  {'A04V', "Enfeeble", 'Oshd', 0, "BTNCurse"},
  {'A0GK', "Brain Sap", 'Oshd', 1, "BTNDevourMagic"},
  {'A04Y', "Nightmare", 'Oshd', 2, "BTNWandOfShadowSight"},
  {'A02Q', "Fiend's Grip", 'Oshd', 3, "BTNAmuletOftheWild"},
  {'A05V', "Death Pulse", 'U00E', 0, "BTNDeathNova"},
  {'A0MC', "Diffusion Aura", 'U00E', 1, "PASBTNShadeTrueSight"},
  {'A060', "Sadist", 'U00E', 2, "BTNOrbOfCorruption"},
  {'A067', "Reaper's Scythe", 'U00E', 3, "BTNINV_Sword_09"},
  {'A08P', "Reaper's Scythe", 'U00E', 3, "BTNINV_Sword_09"},
  {'A06I', "Meat Hook", 'U00F', 0, "BTNImpale"},
  {'A06K', "Rot", 'U00F', 1, "BTNPlagueCloud"},
  {'A06D', "Flesh Heap", 'U00F', 2, "PASBTNExhumeCorpses"},
  {'A0FL', "Dismember", 'U00F', 3, "BTNCannibalize"},
  {'A0ML', "Charge of Darkness", 'O00J', 0, "BTNSpell_Shadow_GatherShadows"},
  {'A0ES', "Empowering Haste", 'O00J', 1, "BTNEtherealFormOn"},
  {'A0G5', "Greater Bash", 'O00J', 2, "BTNBash1"},
  {'A0G4', "Nether Strike", 'O00J', 3, "BTNSacrifice"},
  {'A00T', "Watcher", 'Ubal', 0, "BTNWandOfShadowSight"},
  {'A0CA', "Shukuchi", 'Ubal', 1, "BTNWeaverShukuchi"},
  {'A0CG', "Geminate Attack", 'Ubal', 2, "BTNLocustSwarm"},
  {'A0CT', "Time Lapse", 'Ubal', 3, "BTNOrbOfFrost"},
  {'A0EY', "Shadowraze", 'Nfir', 0, "BTNDeathCoil"},
  {'A0BR', "Necromastery", 'Nfir', 1, "BTNSacrificialSkull"},
  {'A0FU', "Presence of the Dark Lord", 'Nfir', 2, "PASBTNRegenerationAura"},
  {'A0HE', "Requiem of Souls", 'Nfir', 3, "BTNDizzy"},
  {'A06O', "Burrowstrike", 'U00K', 0, "BTNImpale"},
  {'A0H0', "Sand Storm", 'U00K', 1, "BTNSandStorm"},
  {'A0FA', "Caustic Finale", 'U00K', 2, "PASBTNPoisonSting"},
  {'A06R', "Epicenter", 'U00K', 3, "BTNEarthquake"},
  {'A0I6', "Berserker's Call", 'Opgh', 0, "BTN_BerserkerCall"},
  {'A0S1', "Battle Hunger", 'Opgh', 1, "BTNIncinerate"},
  {'A0C6', "Counter Helix", 'Opgh', 2, "BTNStaffOfSanctuary"},
  {'A0E2', "Culling Blade", 'Opgh', 3, "BTNOrcMeleeUpThree"},
  {'A0EC', "Bloodrage", 'Hvsh', 0, "BTNSpellSteal"},
  {'A0LE', "Blood Bath", 'Hvsh', 1, "BTNSpell_Shadow_LifeDrain"},
  {'A0I8', "Strygwyr's Thirst", 'Hvsh', 2, "BTNThirst"},
  {'A0LH', "Rupture", 'Hvsh', 3, "BTNMarkOfFire"},
  {'A0I3', "Death Coil", 'Udea', 0, "BTNDeathCoil"},
  {'A0MF', "Aphotic Shield", 'Udea', 1, "BTNLightningShield"},
  {'A0MG', "Frostmourne", 'Udea', 2, "BTNIceBlade"},
  {'A0NS', "Borrowed Time", 'Udea', 3, "BTNAnimateDead"},
  {'A0HW', "Spectral Dagger", 'E01B', 0, "BTNAbility_Spectral_Dagger"},
  {'A0FX', "Desolate", 'E01B', 1, "PASBTNSciophobia"},
  {'A0NA', "Dispersion", 'E01B', 2, "PASBTNThickFur"},
  {'A0H9', "Haunt", 'E01B', 3, "BTNHaunt"},
  {'A0NM', "Paralyzing Cask", 'E01A', 0, "BTNAcidBomb"},
  {'A0NE', "Voodoo Restoration", 'E01A', 1, "BTNBigBadVoodooSpell"},
  {'A0NO', "Maledict", 'E01A', 2, "BTNShadowPact"},
  {'A0NT', "Death Ward", 'E01A', 3, "BTNStasisTrap"},
  {'A0NX', "Death Ward", 'E01A', 3, "BTNStasisTrap"},
  {'A0OI', "Arcane Orb", 'U00P', 0, "BTNOrbOfDeathOn"},
  {'A0OJ', "Astral Imprisonment", 'U00P', 1, "BTNSpell_Shadow_AntiShadow"},
  {'A0OG', "Essence Aura", 'U00P', 2, "PASBTNScatterRockets"},
  {'A0OK', "Sanity's Eclipse", 'U00P', 3, "BTNSpell_Shadow_ImpPhaseShift"},
  {'A0J5', "Fatal Bonds", 'E01C', 0, "BTNBoneChimes"},
  {'A0AS', "Shadow Word", 'E01C', 1, "BTNSpell_Shadow_ShadowWordPain"},
  {'A06P', "Upheaval", 'E01C', 2, "BTNUnsummonBuilding"},
  {'S008', "Rain of Chaos", 'E01C', 3, "BTNInfernal"},
  {'A0NB', "Earthbind", 'H00I', 0, "BTNQuillSpray"},
  {'A0N8', "Poof", 'H00I', 1, "BTNDispelMagic"},
  {'A0N7', "Geostrike", 'H00I', 2, "PASBTNPillage"},
  {'A0MW', "Divided We Stand", 'H00I', 3, "BTNUpgradeRegenerationAura"},
  {'A0NQ', "Poison Touch", 'N01W', 0, "BTNINV_Wand_12"},
  {'A0NV', "Weave", 'N01W', 1, "BTNSpell_Shadow_ShadowWard"},
  {'A0OR', "Shadow Wave", 'N01W', 2, "BTNShadowWave"},
  {'A0OS', "Shallow Grave", 'N01W', 3, "BTNAnkh"},
  {'A01I', "Firestorm", 'N00R', 0, "BTNFire"},
  {'A0RA', "Pit of Malice", 'N00R', 1, "BTNGrave"},
  {'A0QT', "Expulsion", 'N00R', 2, "BTNWallOfFire"},
  {'A0R0', "Dark Rift", 'N00R', 3, "BTNDarkPortal"},
  {'A0QV', "Raise Dead", 'H00R', 0, "BTNDalaranMutant"},
  {'A0R5', "Soul Rip", 'H00R', 1, "BTNHeartStopper"},
  {'A01N', "Heartstopper Aura", 'H00R', 2, "PASBTNHS"},
  {'A0R3', "Plague", 'H00R', 3, "BTNSpell_Shadow_FingerOfDeath"},
  {'A0QE', "Vacuum", 'H00N', 0, "BTNSpell_Shadow_Teleport"},
  {'A0QG', "Ion Shell", 'H00N', 1, "BTNSpell_Shadow_Haunting"},
  {'A0R7', "Surge", 'H00N', 2, "BTNSpell_Nature_AstralRecal"},
  {'A0QK', "Wall of Replica", 'H00N', 3, "BTNWallRep"},
  {'A0VB', "Quas", 'H00U', 0, "BTNOrbOfFrost"},
  {'A0VA', "Wex", 'H00U', 1, "BTNOrbOfLightning"},
  {'A0V9', "Exort", 'H00U', 2, "BTNOrbOfFire"},
  {'A0VF', "Invoke", 'H00U', 3, "BTNInvoke"},

  {'A0JQ', "Feast", 'U00C', 0, "BTNCannibalize"},
  {'A01E', "Poison Sting", 'U00C', 1, "BTNPoisonGloves"},
  {'A06Y', "Anabolic Frenzy", 'U00C', 2, "BTNGhoulFrenzy"},
  {'A028', "Rage", 'U00C', 3, "BTNReincarnation"},

  {'Aamk', "Attribute Bonus", 0, 4, "BTNStatUp"},
  {'A0NR', "Attribute Bonus", 0, 4, "BTNStatUp"}
};
static DotaAbility abilityBackup[sizeof abilities / sizeof abilities[0]];

int getNumHeroes ()
{
  return sizeof heroes / sizeof heroes[0];
}
DotaHero* getHero (int i)
{
  if (i < 0 || i >= getNumHeroes ())
    return NULL;
  heroes[i].index = i;
  return &heroes[i];
}
DotaHero* getHeroByName (char const* n)
{
  for (int i = getNumHeroes () - 1; i >= 0; i--)
    if (!stricmp (n, heroes[i].name))
    {
      heroes[i].index = i;
      return &heroes[i];
    }
  return NULL;
}
DotaHero* getHeroById (int id)
{
  for (int i = getNumHeroes () - 1; i >= 0; i--)
    if (heroes[i].id == id)
    {
      heroes[i].index = i;
      return &heroes[i];
    }
  return NULL;
}

int getNumItems ()
{
  return sizeof items / sizeof items[0];
}
DotaItem* getItem (int i)
{
  if (i < 0 || i >= getNumItems ())
    return NULL;
  items[i].index = i;
  return &items[i];
}
DotaItem* getItemByName (char const* n)
{
  for (int i = getNumItems () - 1; i >= 0; i--)
    if (!stricmp (n, items[i].name) && items[i].id != 0)
    {
      items[i].index = i;
      return &items[i];
    }
  return NULL;
}
DotaItem* getCombinedItem (char const* n)
{
  for (int i = getNumItems () - 1; i >= 0; i--)
    if (!stricmp (n, items[i].name) && items[i].id == 0)
    {
      items[i].index = i;
      return &items[i];
    }
  return NULL;
}
DotaItem* getItemById (int id)
{
  for (int i = getNumItems () - 1; i >= 0; i--)
    if (items[i].id == id || items[i].alt == id)
    {
      items[i].index = i;
      return &items[i];
    }
  return NULL;
}

int getNumAbilities ()
{
  return sizeof abilities / sizeof abilities[0];
}
DotaAbility* getAbility (int i)
{
  if (i < 0 || i >= getNumAbilities ())
    return NULL;
  abilities[i].index = i;
  return &abilities[i];
}
DotaAbility* getAbilityById (int id)
{
  DotaAbility* best = NULL;
  for (int i = getNumAbilities () - 1; i >= 0; i--)
    if (abilities[i].id == id)
    {
      abilities[i].index = i;
      best = &abilities[i];
    }
  return best;
}
DotaAbility* getAbilityById (int id, int hero)
{
  DotaAbility* best = NULL;
  hero = getHero (hero)->id;
  for (int i = getNumAbilities () - 1; i >= 0; i--)
    if (abilities[i].id == id && (abilities[i].hero == 0 || abilities[i].hero == hero))
    {
      abilities[i].index = i;
      best = &abilities[i];
    }
  return best;
}
DotaAbility* getHeroAbility (int id, int slot)
{
  for (int i = getNumAbilities () - 1; i >= 0; i--)
    if (abilities[i].slot == slot && (slot == 4 || abilities[i].hero == id))
    {
      abilities[i].index = i;
      return &abilities[i];
    }
  return NULL;
}

#include "replay.h"

void fixVersion (DotaData* dota)
{
  static bool first = true;
  if (first)
  {
    memcpy (itemBackup, items, sizeof items);
    memcpy (abilityBackup, abilities, sizeof abilities);
    first = false;
  }
  else
  {
    memcpy (items, itemBackup, sizeof items);
    memcpy (abilities, abilityBackup, sizeof abilities);
  }
  if (dota->major != 6) return;
  if (dota->minor < 52)
  {
    getItemByName ("Sange")->cost = 600;
    getItemByName ("Yasha")->cost = 600;
    getItemByName ("Sange and Yasha")->cost = 1000;
  }
  if (dota->minor < 51)
    getItemByName ("Buriza-do Kyanon")->cost = 1250;
  if (dota->minor < 50)
  {
    getItemByName ("Ultimate Orb")->cost = 2250;
    getItemByName ("Quarterstaff")->cost = 1150;
    getItemByName ("Void Stone")->cost = 900;
    getItemByName ("Lothar's Edge")->cost = 1400;
    getItemByName ("Vladmir's Offering")->cost = 625;
  }
  if (dota->minor < 49)
  {
    getItemByName ("Ultimate Orb")->cost = 2300;
    getItemByName ("Vladmir's Offering")->cost = 1000;
  }
  if (dota->minor < 46 || (dota->minor == 46 && dota->build < 1))
    getItemByName ("Null Talisman")->cost = 175;
  if (dota->minor < 44)
    getItemByName ("Ancient Tango of Essifation")->cost = 80;
  if (dota->minor < 42)
    getItemByName ("The Butterfly")->cost = 1400;
  if (dota->minor < 38)
  {
    getItemByName ("Hyperstone")->cost = 2450;
    getItemByName ("Wraith Band")->cost = 150;
  }
  if (dota->minor < 36)
  {
    getItemByName ("Crystalis")->cost = 1000;
    getItemByName ("Buriza-do Kyanon")->cost = 750;
  }
  if (dota->minor < 35)
    getItemByName ("Stygian Desolator")->cost = 1400;
  if (dota->minor == 33 && dota->build < 1)
  {
    getItemByName ("Eul's Scepter of Divinity")->cost = 800;
    getItemByName ("Guinsoo's Scythe of Vyse")->cost = 100;
  }
  if (dota->minor < 33)
  {
    getItemByName ("Eul's Scepter of Divinity")->cost = 0;
    getItemByName ("Guinsoo's Scythe of Vyse")->cost = 900;
    getItemByName ("Stygian Desolator")->cost = 1500;
  }
  if (dota->minor < 31)
    getItemByName ("Mask of Madness")->cost = 1100;
  if (dota->minor < 28)
  {
    getItemByName ("Stygian Desolator")->cost = 1580;
    getItemByName ("Sange and Yasha")->cost = 900;
  }
  if (dota->minor >= 50)
    strcpy (getItemByName ("Kelen's Dagger of Escape")->name, "Kelen's Dagger");
  if (dota->minor < 40)
  {
    for (int i = 0; i < getNumAbilities (); i++)
    {
      if (!stricmp (abilities[i].name, "Blink Strike"))
        abilities[i].hero = 0;
    }
  }
}

static char const scrollImage[] = "BTNSnazzyScroll";
char const* getItemIcon (int i)
{
  if (i <= getItemByName ("Vitality Booster")->index || items[i].id == 0)
    return items[i].imgTag;
  else
    return scrollImage;
}

DWORD getSlotColor (int clr)
{
  switch (clr)
  {
  case 0:
    return RGB (255, 2, 2);
  case 1:
    return RGB (0, 65, 255);
  case 2:
    return RGB (27, 229, 184);
  case 3:
//    return RGB (83, 0, 128);
    return RGB (166, 0, 255);
  case 4:
//    return RGB (255, 252, 0);
    return RGB (214, 210, 0);
  case 5:
    return RGB (254, 137, 13);
  case 6:
    return RGB (31, 191, 0);
  case 7:
    return RGB (228, 90, 175);
  case 8:
    return RGB (148, 149, 150);
  case 9:
    return RGB (125, 190, 241);
  case 10:
    return RGB (15, 97, 69);
  case 11:
//    return RGB (77, 41, 3);
    return RGB (141, 77, 5);
  default:
    return RGB (255, 255, 255);
  }
}

DWORD getDefaultColor (int clr)
{
  switch (clr)
  {
  case 0:
    return RGB (255, 2, 2);
  case 1:
    return RGB (0, 65, 255);
  case 2:
    return RGB (27, 229, 184);
  case 3:
    return RGB (83, 0, 128);
  case 4:
    return RGB (255, 252, 0);
  case 5:
    return RGB (254, 137, 13);
  case 6:
    return RGB (31, 191, 0);
  case 7:
    return RGB (228, 90, 175);
  case 8:
    return RGB (148, 149, 150);
  case 9:
    return RGB (125, 190, 241);
  case 10:
    return RGB (15, 97, 69);
  case 11:
    return RGB (77, 41, 3);
  default:
    return RGB (255, 255, 255);
  }
}

DWORD getLightColor (int clr)
{
  DWORD c = getSlotColor (clr);
  for (int i = 0; i < 24; i += 8)
    c = (c & (~(0xFF << i))) | ((((((c >> i) & 0xFF) + 510) / 3) & 0xFF) << i);
  return c;
}
DWORD getDarkColor (int clr)
{
  DWORD c = getDefaultColor (clr);
  if ((c & 0xFF) + ((c >> 8) & 0xFF) + ((c >> 16) & 0xFF) > 383)
  {
    for (int i = 0; i < 24; i += 8)
      c = (c & (~(0xFF << i))) | (((((c >> i) & 0xFF) * 2 / 3) & 0xFF) << i);
  }
  return c;
}

DWORD getFlipColor (int clr)
{
  DWORD c = getSlotColor (clr);
  return ((c & 0x0000FF) << 16) | (c & 0x00FF00) | ((c & 0xFF0000) >> 16);
}

DotaTavern taverns[] = {
  {"Morning Tavern", 0},
  {"Sunrise Tavern", 0},
  {"Dawn Tavern", 0},
  {"Light Tavern", 0},

  {"Midnight Tavern", 1},
  {"Evening Tavern", 1},
  {"Twilight Tavern", 1},
  {"Dusk Tavern", 1}
};

DotaTavern* getTavern (int i)
{
  return &taverns[i];
}

static TrieNode* odata = NULL;
static int translateFrom[8192];
static int translateTo[8192];
static const int hashSize = 8191;

void setEquiv (int from, int to)
{
  int cur = (from % hashSize);
  while (translateFrom[cur])
    cur = (cur + 1) % hashSize;
  translateFrom[cur] = from;
  translateTo[cur] = to;
}
void setEquiv (char const* from, int to)
{
  int id = getValue (odata, from);
  if (id != 0)
    setEquiv (to, id);
}

void resetVersion ()
{
  if (odata == NULL)
  {
    //for (int i = 0; i < getNumHeroes (); i++)
    //  odata = addString (odata, heroes[i].name, heroes[i].id);
    //for (int i = 0; i < getNumItems (); i++)
    //  odata = addString (odata, items[i].name, items[i].id);
    for (int i = 0; i < getNumAbilities (); i++)
    {
      odata = addString (odata, abilities[i].name, abilities[i].id);
      abilities[i].rhero = abilities[i].hero;
    }
  }
  memset (translateFrom, 0, sizeof translateFrom);
  setEquiv ('N017', 'N016');
  setEquiv ('N013', 'N010');
  setEquiv ('N014', 'N010');
  setEquiv ('U007', 'U00C');
  for (int i = 0; i < getNumAbilities (); i++)
    abilities[i].hero = abilities[i].rhero;
}
void zero_abils ()
{
  for (int i = 0; i < getNumAbilities (); i++)
    abilities[i].hero = 0;
}
void set_abils (unsigned long id, char const* list)
{
  fix_itemid (id);
  int len = (int) strlen (list);
  for (int i = 0; i <= len - 4; i += 5)
  {
    unsigned long iid = makeID (list + i);
    fix_itemid (iid);
    DotaAbility* abil = getAbilityById (iid);
    if (abil && abil->rhero)
      abil->hero = id;
  }
}

void fix_itemid (unsigned long& id)
{
  int cur = (id % hashSize);
  while (translateFrom[cur])
  {
    if (translateFrom[cur] == id)
    {
      id = translateTo[cur];
      return;
    }
    cur = (cur + 1) % hashSize;
  }
}

void delDotaData ()
{
  delete odata;
}
