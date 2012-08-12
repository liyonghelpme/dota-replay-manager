#ifndef __DOTA_LOAD_H__
#define __DOTA_LOAD_H__

#include "dota.h"
#include "utils.h"
#include "rmpq.h"
#include "dotareplay.h"
#include "datafile.h"

extern DotaHero heroes[256];
extern DotaItem items[512];
extern DotaAbility abilities[1024];
extern DotaTavern taverns[32];
extern DotaShop shops[32];
extern DotaRecipe recipes[256];
extern int numHeroes;
extern int numItems;
extern int numAbilities;
extern int numTaverns;
extern int numShops;
extern int numRecipes;
extern DotaHero bk_heroes[256];
extern DotaItem bk_items[256];
extern int bk_numHeroes;
extern int bk_numItems;
void resetData ();
extern bool useropen;

#endif
