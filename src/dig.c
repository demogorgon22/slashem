/*	SCCS Id: @(#)dig.c	3.3	2000/04/19	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
/* #define DEBUG */	/* turn on for diagnostics */

#ifdef OVLB

static NEARDATA boolean did_dig_msg;

STATIC_DCL boolean NDECL(rm_waslit);
STATIC_DCL void FDECL(mkcavepos, (XCHAR_P,XCHAR_P,int,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void FDECL(mkcavearea, (BOOLEAN_P));
STATIC_DCL int FDECL(dig_typ, (XCHAR_P,XCHAR_P));
STATIC_DCL int NDECL(dig);
STATIC_DCL schar FDECL(fillholetyp, (int, int));
STATIC_DCL void NDECL(dig_up_grave);


STATIC_OVL boolean
rm_waslit()
{
    register xchar x, y;

    if(levl[u.ux][u.uy].typ == ROOM && levl[u.ux][u.uy].waslit)
	return(TRUE);
    for(x = u.ux-2; x < u.ux+3; x++)
	for(y = u.uy-1; y < u.uy+2; y++)
	    if(isok(x,y) && levl[x][y].waslit) return(TRUE);
    return(FALSE);
}

/* Change level topology.  Messes with vision tables and ignores things like
 * boulders in the name of a nice effect.  Vision will get fixed up again
 * immediately after the effect is complete.
 */
STATIC_OVL void
mkcavepos(x, y, dist, waslit, rockit)
    xchar x,y;
    int dist;
    boolean waslit, rockit;
{
    register struct rm *lev;

    if(!isok(x,y)) return;
    lev = &levl[x][y];

    if(rockit) {
	register struct monst *mtmp;

	if(IS_ROCK(lev->typ)) return;
	if(t_at(x, y)) return; /* don't cover the portal */
	if ((mtmp = m_at(x, y)) != 0)   /* make sure crucial monsters survive */
	    if(!passes_walls(mtmp->data)) rloc(mtmp);
    } else if(lev->typ == ROOM) return;

    unblock_point(x,y); /* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if(dist < 3) lev->lit = (rockit ? FALSE : TRUE);
    if(waslit) lev->waslit = (rockit ? FALSE : TRUE);
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 3 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;
    lev->typ = (rockit ? STONE : ROOM);
    if(dist >= 3)
	impossible("mkcavepos called with dist %d", dist);
    if(Blind)
	feel_location(x, y);
    else newsym(x,y);
}

STATIC_OVL void
mkcavearea(rockit)
register boolean rockit;
{
    int dist;
    xchar xmin = u.ux, xmax = u.ux;
    xchar ymin = u.uy, ymax = u.uy;
    register xchar i;
    register boolean waslit = rm_waslit();

    if(rockit) pline("Crash!  The ceiling collapses around you!");
    else pline("A mysterious force %s cave around you!",
	     (levl[u.ux][u.uy].typ == CORR) ? "creates a" : "extends the");
    display_nhwindow(WIN_MESSAGE, TRUE);

    for(dist = 1; dist <= 2; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist < 2) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkcavepos(i, ymin, dist, waslit, rockit);
		mkcavepos(i, ymax, dist, waslit, rockit);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkcavepos(xmin, i, dist, waslit, rockit);
	    mkcavepos(xmax, i, dist, waslit, rockit);
	}

	flush_screen(1);        /* make sure the new glyphs shows up */
	delay_output();
    }

    if(!rockit && levl[u.ux][u.uy].typ == CORR) {
	levl[u.ux][u.uy].typ = ROOM;
	if(waslit) levl[u.ux][u.uy].waslit = TRUE;
	newsym(u.ux, u.uy); /* in case player is invisible */
    }

    vision_full_recalc = 1;     /* everything changed */
}

/* When digging into location <x,y>, what are you actually digging into? */
/* result: 1=>statue, 2=>boulder, 3=>door, 0=>other; used as array index */
/* KMH -- Added 4=>tree */
STATIC_OVL int
dig_typ(x, y)
xchar x, y;
{
	return (sobj_at(STATUE, x, y) ? 1 :
		sobj_at(BOULDER, x, y) ? 2 :
		closed_door(x, y) ? 3 :
		IS_TREE(levl[x][y].typ) ? 4: 0);
}

#define BY_YOU          (&youmonst)
#define BY_OBJECT       ((struct monst *)0)

boolean
dig_check(madeby, verbose, x, y)
	struct monst    *madeby;
	boolean         verbose;
	int             x, y;
{
	struct trap *ttmp = t_at(x, y);

	if (On_stairs(x, y)) {
	    if (x == xdnladder || x == xupladder) {
		if(verbose) pline_The("ladder resists your effort.");
	    } else if(verbose) pline_The("stairs are too hard to %s into.",
	    			    (is_lightsaber(uwep) ? "cut": "dig"));
	    return(FALSE);
#ifdef DEVEL_BRANCH
	/* ALI - Artifact doors */
	} else if (IS_DOOR(levl[x][y].typ) && artifact_door(x, y)) {
	    if(verbose) pline_The("%s here is too hard to dig in.",
				  surface(x,y));
	    return(FALSE);
#endif
	} else if (IS_THRONE(levl[x][y].typ) && madeby != BY_OBJECT) {
	    if(verbose) pline_The("throne is too hard to break apart.");
	    return(FALSE);
	} else if (IS_ALTAR(levl[x][y].typ) && (madeby != BY_OBJECT ||
				Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
	    if(verbose) pline_The("altar is too hard to break apart.");
	    return(FALSE);
	} else if (Is_airlevel(&u.uz)) {
	    if(verbose) You("cannot dig in thin air.");
	    return(FALSE);
	} else if (Is_waterlevel(&u.uz)) {
	    if(verbose) pline_The("water splashes and subsides.");
	    return(FALSE);
 	} else if ((IS_WALL(levl[x][y].typ) &&
 		      (levl[x][y].wall_info & W_NONDIGGABLE) != 0)
 		|| (ttmp &&
 		      (ttmp->ttyp == MAGIC_PORTAL || !Can_dig_down(&u.uz)))) {
	    if(verbose) pline_The("%s here is too hard to dig in.",
				  surface(x,y));
	    return(FALSE);
	} else if (sobj_at(BOULDER, x, y)) {
	    if(verbose) There("isn't enough room to dig here.");
	    return(FALSE);
 	} else if (madeby == BY_OBJECT &&
 		    /* the block against existing traps is mainly to
 		       prevent broken wands from turning holes into pits */
 		    (ttmp || is_pool(x,y) || is_lava(x,y))) {
	    /* digging by player handles pools separately */
	    return FALSE;
	}
	return(TRUE);
}

STATIC_OVL int
dig()
{
	register struct rm *lev;
	register xchar dpx = digging.pos.x, dpy = digging.pos.y;
	int bonus;

	lev = &levl[dpx][dpy];
	/* perhaps a nymph stole your pick-axe while you were busy digging */
	/* or perhaps you teleported away */
	/* KMH, balance patch -- allow all picks */
	/* WAC allow axes and lightsabers */
	if (u.uswallow || !uwep || 
		(!is_pick(uwep) && (!is_lightsaber(uwep) || !uwep->lamplit) &&
		 !is_axe(uwep)) ||
		!on_level(&digging.level, &u.uz) ||
		((digging.down ? (dpx != u.ux || dpy != u.uy)
			   : (distu(dpx,dpy) > 2))))
		return(0);

	if (digging.down) {
	    if(!dig_check(BY_YOU, TRUE, u.ux, u.uy)) return(0);
	} else { /* !digging.down */
#ifdef DEVEL_BRANCH
	    /* ALI - Artifact doors */
	    if (IS_ROCK(lev->typ) && !may_dig(dpx,dpy) && !dig_typ(dpx, dpy) ||
		    IS_DOOR(lev->typ) && artifact_door(dpx, dpy)) {
		pline("This %s is too hard to %s.",
			IS_DOOR(lev->typ) ? "door" : "wall",
#else
	    if (IS_ROCK(lev->typ) && !may_dig(dpx,dpy) && !dig_typ(dpx, dpy)) {
		pline("This wall is too hard to %s.", 
#endif
			(is_lightsaber(uwep) ? "cut through" : "dig into"));
		return(0);
	    }
	    /* KMH -- Added tree */
	    if (IS_TREE(lev->typ) && !may_dig(dpx,dpy) && dig_typ(dpx, dpy) == 4) {
		pline("This tree seems to be petrified.");
		return(0);
	    }
	    if (IS_TREE(lev->typ) && !may_dig(dpx,dpy) && dig_typ(dpx, dpy) == 4) {
		pline("This tree seems to be petrified.");
		return(0);
	    }
	}
	/* Can't exactly miss holding a lightsaber to the wall */
	if(Fumbling && !rn2(3) && !is_lightsaber(uwep)) {
		switch(rn2(3)) {
		case 0:  if(!welded(uwep)) {
			     You("fumble and drop your %s.", xname(uwep));
			     dropx(uwep);
			     setuwep((struct obj *)0);
			 } else {
			     pline("Ouch!  Your %s bounces and hits you!",
				xname(uwep));
			     set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
			 }
			 break;
		case 1:  pline("Bang!  You hit with the broad side of %s!",
			       the(xname(uwep)));
			 break;
		default: Your("swing misses its mark.");
			 break;
		}
		return(0);
	}

	bonus = 10 + rn2(5) + abon() + uwep->spe - greatest_erosion(uwep) + u.udaminc;
	if (Race_if(PM_DWARF))
	    bonus *= 2;
	if (is_lightsaber(uwep))
	    bonus -= rn2(20); /* Melting a hole takes longer */

	digging.effort += bonus;

	if (digging.down) {
		register struct trap *ttmp;

		if (digging.effort > 250) {
		    (void) dighole(FALSE);
		    (void) memset((genericptr_t)&digging, 0, sizeof digging);
		    return(0);  /* done with digging */
		}

		if (digging.effort <= 50 ||
		    (is_lightsaber(uwep)) ||
		    ((ttmp = t_at(dpx,dpy)) != 0 &&
			(ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT ||
			 ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)))
		    return(1);

		if (IS_ALTAR(lev->typ)) {
		    altar_wrath(dpx, dpy);
		    angry_priest();
		}

		if (dighole(TRUE)) {    /* make pit at <u.ux,u.uy> */
		    digging.level.dnum = 0;
		    digging.level.dlevel = -1;
		}
		return(0);
	}

	if (digging.effort > 100) {
		register const char *digtxt, *dmgtxt = (const char*) 0;
		register struct obj *obj;
		register boolean shopedge = *in_rooms(dpx, dpy, SHOPBASE);

		if ((obj = sobj_at(STATUE, dpx, dpy)) != 0) {
			if (break_statue(obj))
				digtxt = "The statue shatters.";
			else
				/* it was a statue trap; break_statue()
				 * printed a message and updated the screen
				 */
				digtxt = (char *)0;
		} else if ((obj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			fracture_rock(obj);
			digtxt = "The boulder falls apart.";
		} else if (lev->typ == STONE || lev->typ == SCORR ||
				IS_TREE(lev->typ)) {
			if(Is_earthlevel(&u.uz)) {
			    if(uwep->blessed && !rn2(3)) {
				mkcavearea(FALSE);
				goto cleanup;
			    } else if((uwep->cursed && !rn2(4)) ||
					  (!uwep->blessed && !rn2(6))) {
				mkcavearea(TRUE);
				goto cleanup;
			    }
			}
			if (IS_TREE(lev->typ)) {
				digtxt = "You cut down the tree.";
				lev->typ = ROOM;
			} else {
				digtxt = "You succeed in cutting away some rock.";
				lev->typ = CORR;
			}
		} else if(IS_WALL(lev->typ)) {
			if(shopedge) {
			    add_damage(dpx, dpy, 10L * ACURRSTR);
			    dmgtxt = "damage";
			}
			if (level.flags.is_maze_lev) {
			    lev->typ = ROOM;
			} else if (level.flags.is_cavernous_lev) {
			    lev->typ = CORR;
			} else {
			    lev->typ = DOOR;
			    lev->doormask = D_NODOOR;
			}
			digtxt = "You make an opening in the wall.";
		} else if(lev->typ == SDOOR) {
 			cvt_sdoor_to_door(lev);	/* ->typ = DOOR */
			digtxt = "You break through a secret door!";
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else if(closed_door(dpx, dpy)) {
			digtxt = "You break through the door.";
			if(shopedge) {
			    add_damage(dpx, dpy, 400L);
			    dmgtxt = "break";
			}
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else return(0); /* statue or boulder got taken */

		unblock_point(dpx,dpy); /* vision:  can see through */
		if(Blind)
		    feel_location(dpx, dpy);
		else
		    newsym(dpx, dpy);
		if(digtxt) pline(digtxt);       /* after newsym */
		if(dmgtxt)
		    pay_for_damage(dmgtxt);

		if(Is_earthlevel(&u.uz) && !rn2(3)) {
		    register struct monst *mtmp;

		    switch(rn2(2)) {
		      case 0:
			mtmp = makemon(&mons[PM_EARTH_ELEMENTAL],
					dpx, dpy, NO_MM_FLAGS);
			break;
		      default:
			mtmp = makemon(&mons[PM_XORN],
					dpx, dpy, NO_MM_FLAGS);
			break;
		    }
		    if(mtmp) pline_The("debris reassembles and comes to life!");
		}
		if(IS_DOOR(lev->typ) && (lev->doormask & D_TRAPPED)) {
			lev->doormask = D_NODOOR;
			b_trapped("door", 0);
			newsym(dpx, dpy);
		}
cleanup:
		digging.level.dnum = 0;
		digging.level.dlevel = -1;
		return(0);
	} else {                /* not enough effort has been spent yet */
		/* KMH -- Added tree */
		static const char *d_target[5] = {
					"rock", "statue", "boulder", "door", "tree"
		};
		int dig_target = dig_typ(dpx, dpy);

		if (IS_WALL(lev->typ) || dig_target == 3) {
		    if(*in_rooms(dpx, dpy, SHOPBASE)) {
			pline("This %s seems too hard to dig into.",
			      IS_DOOR(lev->typ) ? "door" : "wall");
			return(0);
		    }
		} else if (!IS_ROCK(lev->typ) && !dig_target)
			return(0); /* statue or boulder got taken */
		if(!did_dig_msg) {
		    if (is_lightsaber(uwep)) You("burn steadily through %s.",
			d_target[dig_target]);
		    else You("hit the %s with all your might.",
			d_target[dig_target]);
		    did_dig_msg = TRUE;
		}
	}
	return(1);
}

/* When will hole be finished? Very rough indication used by shopkeeper. */
int
holetime()
{
	if(occupation != dig || !*u.ushops) return(-1);
	return ((250 - digging.effort) / 20);
}

/* Return typ of liquid to fill a hole with, or ROOM, if no liquid nearby */
STATIC_OVL
schar
fillholetyp(x,y)
int x, y;
{
    register int x1, y1;
    int lo_x = max(1,x-1), hi_x = min(x+1,COLNO-1),
	lo_y = max(0,y-1), hi_y = min(y+1,ROWNO-1);
    int pool_cnt = 0, moat_cnt = 0, lava_cnt = 0;

    for (x1 = lo_x; x1 <= hi_x; x1++)
	for (y1 = lo_y; y1 <= hi_y; y1++)
	    if (levl[x1][y1].typ == POOL)
		pool_cnt++;
	    else if (levl[x1][y1].typ == MOAT ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_MOAT))
		moat_cnt++;
	    else if (levl[x1][y1].typ == LAVAPOOL ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_LAVA))
		lava_cnt++;
    pool_cnt /= 3;              /* not as much liquid as the others */

    if (lava_cnt > moat_cnt + pool_cnt && rn2(lava_cnt + 1))
	return LAVAPOOL;
    else if (moat_cnt > 0 && rn2(moat_cnt + 1))
	return MOAT;
    else if (pool_cnt > 0 && rn2(pool_cnt + 1))
	return POOL;
    else
	return ROOM;
}

void
digactualhole(x, y, madeby, ttyp)
register int    x, y;
struct monst    *madeby;
int ttyp;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);        /* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);
	boolean wont_fall = Levitation || Flying;

	/* these furniture checks were in dighole(), but wand
	   breaking bypasses that routine and calls us directly */
	if (IS_FOUNTAIN(lev->typ)) {
	    dogushforth(FALSE);
	    lev->looted |= F_WARNED;		/* force dryup */
	    dryup(x, y, madeby_u);
	    return;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
	    breaksink(x, y);
	    return;
	} else if (IS_TOILET(lev->typ)) {
		breaktoilet(u.ux,u.uy);
#endif
	}

	if (ttyp != PIT && !Can_dig_down(&u.uz)) {
	    impossible("digactualhole: can't dig %s on this level.",
		       defsyms[trap_to_defsym(ttyp)].explanation);
	    ttyp = PIT;
	}

	Strcpy(surface_type, surface(x,y));     /* maketrap() might change it */
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[x][y];
	ttmp->tseen = (madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	if (ttyp == PIT) {

	    if(madeby_u) {
		You("dig a pit in the %s.", surface_type);
		if (shopdoor) pay_for_damage("ruin");
	    } else if (!madeby_obj && canseemon(madeby))
		pline("%s digs a pit in the %s.", Monnam(madeby), surface_type);
	    else if (cansee(x, y) && flags.verbose)
		pline("A pit appears in the %s.", surface_type);

	    if(at_u) {
		if (!wont_fall) {
			u.utrap = rn1(4,2);
			u.utraptype = TT_PIT;
			vision_full_recalc = 1; /* vision limits change */
		} else
			u.utrap = 0;
		if (oldobjs != newobjs) /* something unearthed */
			(void) pickup(1);	/* detects pit */
	    } else if(mtmp) {
		if(is_flyer(mtmp->data) || is_floater(mtmp->data)) {
		    if(canseemon(mtmp))
			pline("%s %s over the pit.", Monnam(mtmp),
						     (is_flyer(mtmp->data)) ?
						     "flies" : "floats");
		} else if(mtmp != madeby)
		    (void) mintrap(mtmp);
	    }
	} else {        /* was TRAPDOOR now a HOLE*/

	    if(madeby_u)
		You("dig a hole through the %s.", surface_type);
	    else if(!madeby_obj && canseemon(madeby))
		pline("%s digs a hole through the %s.",
		      Monnam(madeby), surface_type);
	    else if(cansee(x, y) && flags.verbose)
		pline("A hole appears in the %s.", surface_type);

	    if (at_u) {
		if (!u.ustuck && !wont_fall && !next_to_u()) {
		    You("are jerked back by your pet!");
		    wont_fall = TRUE;
		}

		/* Floor objects get a chance of falling down.  The case where
		 * the hero does NOT fall down is treated here.  The case
		 * where the hero does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
		    if (newobjs)
			impact_drop((struct obj *)0, x, y, 0);
		    if (oldobjs != newobjs)
			(void) pickup(1);
		    if (shopdoor && madeby_u) pay_for_damage("ruin");

		} else {
		    d_level newlevel;
		    const char *You_fall = "You fall through...";

		    if (*u.ushops && madeby_u)
			shopdig(1); /* shk might snatch pack */

		    /* Earlier checks must ensure that the destination
		     * level exists and is in the present dungeon.
		     */
		    newlevel.dnum = u.uz.dnum;
		    newlevel.dlevel = u.uz.dlevel + 1;
		    /* Cope with holes caused by monster's actions -- ALI */
		    if (flags.mon_moving) {
			schedule_goto(&newlevel, FALSE, TRUE, FALSE,
			  You_fall, (char *)0);
		    } else {
			pline(You_fall);
			goto_level(&newlevel, FALSE, TRUE, FALSE);
			/* messages for arriving in special rooms */
			spoteffects(FALSE);
		    }
		}
	    } else {
		if (shopdoor && madeby_u) pay_for_damage("ruin");
		if (newobjs)
		    impact_drop((struct obj *)0, x, y, 0);
		if (mtmp) {
		     /*[don't we need special sokoban handling here?]*/
		    if (is_flyer(mtmp->data) || is_floater(mtmp->data) ||
			mtmp->data == &mons[PM_WUMPUS] ||
			(mtmp->wormno && count_wsegs(mtmp) > 5) ||
			mtmp->data->msize >= MZ_HUGE) return;
		    if (mtmp == u.ustuck)       /* probably a vortex */
			    return;             /* temporary? kludge */

		    if (teleport_pet(mtmp, FALSE)) {
			d_level tolevel;

			if (Is_stronghold(&u.uz)) {
			    assign_level(&tolevel, &valley_level);
			} else if (Is_botlevel(&u.uz)) {
			    if (canseemon(mtmp))
				pline("%s avoids the trap.", Monnam(mtmp));
			    return;
			} else {
			    get_level(&tolevel, depth(&u.uz) + 1);
			}
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
		    }
		}
	    }
	}
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
dighole(pit_only)
boolean pit_only;
{
	register struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) ||
#ifdef DEVEL_BRANCH
	   /* ALI - artifact doors */
	   IS_DOOR(levl[u.ux][u.uy].typ) && artifact_door(u.ux, u.uy) ||
#endif
	   (IS_WALL(lev->typ) && (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The("%s here is too hard to dig in.", surface(u.ux,u.uy));

	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		pline_The("%s sloshes furiously for a moment, then subsides.",
			is_lava(u.ux, u.uy) ? "lava" : "water");
		wake_nearby();  /* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (pit_only) {
		    pline_The("drawbridge seems too hard to dig through.");
		    return FALSE;
	} else if (IS_GRAVE(lev->typ)) {        
	    digactualhole(u.ux, u.uy, BY_YOU, PIT);
	    dig_up_grave();
	    return TRUE;
		} else {
		    int x = u.ux, y = u.uy;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&x, &y);
		    destroy_drawbridge(x, y);
		    return TRUE;
		}

	} else if ((boulder_here = sobj_at(BOULDER, u.ux, u.uy)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
		    rn2(2)) {
			pline_The("boulder settles into the pit.");
			ttmp->ttyp = PIT;        /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
			pline("KADOOM! The boulder falls in!");
			(void) delfloortrap(ttmp);
		}
		delobj(boulder_here);
		return TRUE;

	} else if (IS_GRAVE(lev->typ)) {        
			dig_up_grave();
			digactualhole(u.ux, u.uy, BY_YOU, PIT);
			return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
			pline_The("%s here is too hard to dig in.",
			      surface(u.ux, u.uy));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(u.ux, u.uy);

		pline("As you dig, the hole fills with %s!",
		      typ == LAVAPOOL ? "lava" : "water");
		/* KMH, balance patch -- new intrinsic */
		if (!Levitation && !Flying) {
		    if (typ == LAVAPOOL)
			(void) lava_effects();
		    else if (!Wwalking && !Swimming)
			(void) drown();
		}
		return TRUE;

	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
		pline_The("throne is too hard to break apart.");

	} else if (IS_ALTAR(lev->typ)) {
		pline_The("altar is too hard to break apart.");

	} else {
		typ = fillholetyp(u.ux,u.uy);

		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(u.ux, u.uy, BY_YOU, PIT);
		else
			digactualhole(u.ux, u.uy, BY_YOU, HOLE);

		return TRUE;
	}

	return FALSE;
}

STATIC_OVL void
dig_up_grave()
{
	struct obj *otmp;


	/* Grave-robbing is frowned upon... */
	exercise(A_WIS, FALSE);
	if (Role_if(PM_ARCHEOLOGIST)) {
	    adjalign(-sgn(u.ualign.type)*3);
	    You_feel("like a despicable grave-robber!");
	} else if (Role_if(PM_SAMURAI)) {
	    adjalign(-sgn(u.ualign.type));
	    You("disturb the honorable dead!");
	} else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
	    adjalign(-sgn(u.ualign.type));
	    You("have violated the sanctity of this grave!");
	}

	switch (rn2(5)) {
	case 0:
	case 1:
	    You("unearth a corpse.");
	    if (!!(otmp = mk_tt_object(CORPSE, u.ux, u.uy)))
	    	otmp->age -= 100;		/* this is an *OLD* corpse */;
	    break;
	case 2:
	    if (!Blind) pline(Hallucination ? "Dude!  The living dead!" :
 			"The grave's owner is very upset!");
 	    (void) makemon(mkclass(S_ZOMBIE,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	case 3:
	    if (!Blind) pline(Hallucination ? "I want my mummy!" :
 			"You've disturbed a tomb!");
 	    (void) makemon(mkclass(S_MUMMY,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	default:
	    /* No corpse */
	    pline_The("grave seems unused.  Strange....");
	    break;
	}
	levl[u.ux][u.uy].typ = ROOM;
	del_engr_at(u.ux, u.uy);
	newsym(u.ux,u.uy);
	return;
}

int
use_pick_axe(obj)
struct obj *obj;
{
	char dirsyms[12];
	char qbuf[QBUFSZ];
	register char *dsp = dirsyms;
	register const char *sdp;
	register int rx, ry;
	int res = 0;


	if(iflags.num_pad) sdp = ndir; else sdp = sdir;  /* DICE workaround */

	if (obj != uwep) {
	    if (!wield_tool(obj)) return(0);
	    else res = 1;
	}
	if (u.utrap && u.utraptype == TT_WEB) {
	    pline("%s you can't dig while entangled in a web.",
		  /* res==0 => no prior message;
		     res==1 => just got "You now wield a pick-axe." message */
		  !res ? "Unfortunately," : "But");
	    return res;
	}

	while(*sdp) {
		(void) movecmd(*sdp);   /* sets u.dx and u.dy and u.dz */
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(u.dz > 0 || (u.dz == 0 && isok(rx, ry) &&
		    (IS_ROCK(levl[rx][ry].typ) || dig_typ(rx, ry))))
			*dsp++ = *sdp;
		sdp++;
	}
	*dsp = 0;
	Sprintf(qbuf, "In what direction do you want to dig? [%s]", dirsyms);
	if(!getdir(qbuf))
		return(res);
	return use_obj_dig(obj);
}

/* general dig through doors/etc. function
 * Handles pickaxes/lightsabers/axes
 * called from doforce and use_pick_axe
 */
int use_obj_dig(obj)
struct obj *obj;
{
	register struct rm *lev;
	register int rx, ry;
	int dig_target, res = 0, digtyp;
	
	/* This should already be checked... */
	if (obj != uwep) {
	    if (!wield_tool(obj)) return(0);
	    else res = 1;
	}
	
	/* 0 = pick, 1 = lightsaber, 2 = axe */
	digtyp = (is_pick(uwep) ? 0 : is_lightsaber(uwep) ? 1 : 2);

	if (u.uswallow && attack(u.ustuck)) {
		;  /* return(1) */
	} else if (Underwater) {
		pline("Turbulence torpedoes your digging attempts.");
	} else if(u.dz < 0) {
		if(Levitation) {
			if (digtyp == 0) {
			    You("don't have enough leverage.");
			} else {
			    pline_The("ceiling is too hard to %s through.",
				(digtyp == 1 ? "cut" : "chop"));
			}
		} else {
			You_cant("reach the %s.",ceiling(u.ux,u.uy));
		}
	} else if(!u.dx && !u.dy && !u.dz) {
		/* NOTREACHED for lightsabers/axes called from doforce */
		
		char buf[BUFSZ];
		int dam;

		dam = rnd(2) + dbon() + obj->spe;
		if (dam <= 0) dam = 1;
		You("hit yourself with your %s.", aobjnam(obj, (char *)0));
		/* self_pronoun() won't work twice in a sentence */
		if (digtyp == 2) Strcpy(buf, 
			self_pronoun("killed %sself with %%s axe", "him"));
		else if (digtyp == 1) Strcpy(buf, 
			self_pronoun("killed %sself with %%s lightsaber", "him"));
		else Strcpy(buf, self_pronoun("killed %sself with %%s pick",
			"him"));
		losehp(dam, self_pronoun(buf, "his"), NO_KILLER_PREFIX);
		flags.botl=1;
		return(1);
	} else if(u.dz == 0) {
		if(Stunned || (Confusion && !rn2(5))) confdir();
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(!isok(rx, ry)) {
			if (digtyp == 1) pline("Your %s bounces off harmlessly.",
				aobjnam(obj, (char *)0));
			else pline("Clash!");
			return(1);
		}
		lev = &levl[rx][ry];
		if(MON_AT(rx, ry) && attack(m_at(rx, ry)))
			return(1);
		dig_target = dig_typ(rx, ry);
		if (!IS_ROCK(lev->typ) && !dig_target) {
			/* ACCESSIBLE or POOL */
			struct trap *trap = t_at(rx, ry);

			if (trap && trap->ttyp == WEB) {
			    if (!trap->tseen) {
				seetrap(trap);
				There("is a spider web there!");
			    }
			    Your("%s becomes entangled in the web.",
				aobjnam(obj, (char *)0));
			    /* you ought to be able to let go; tough luck */
			    /* (maybe `move_into_trap()' would be better) */
			    nomul(-d(2,2));
			    nomovemsg = "You pull free.";
			} else
			    You("swing your %s through thin air.",
				aobjnam(obj, (char *)0));
		} else if (digtyp == 2 && dig_target != 3 && dig_target != 4) {
			/* Axes only chop through trees and doors */
			You("can't chop through that!");
		} else {
			/* KMH -- Added tree */
			static const char *d_action[5][3] = {
			    {"digging","cutting through the wall",""},
			    {"chipping the statue","cutting the statue",""},
			    {"hitting the boulder","cutting through the boulder",""},
			    {"chopping at the door","burning through the door", 
			    	"chopping through the door"},
			    {"cutting the tree","razing the tree", 
			    	"chopping down the tree"}
			};
			if (digging.pos.x != rx || digging.pos.y != ry ||
			    !on_level(&digging.level, &u.uz) || digging.down) {
			    digging.down = digging.chew = FALSE;
			    digging.pos.x = rx;
			    digging.pos.y = ry;
			    assign_level(&digging.level, &u.uz);
			    digging.effort = 0;
			    You("start %s.", d_action[dig_target][digtyp]);
			} else {
			    You("%s %s.", digging.chew ? "begin" : "continue",
					d_action[dig_target][digtyp]);
			    digging.chew = FALSE;
			}
			did_dig_msg = FALSE;
			if (digtyp == 2)
			   set_occupation(dig, "chopping", 0);
			else if (digtyp == 1)
			   set_occupation(dig, "cutting", 0);
			else set_occupation(dig, "digging", 0);
		}
	} else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
		You("swing your %s through thin air.", aobjnam(obj, (char *)0));
	} else if (!can_reach_floor()) {
		You_cant("reach the %s.", surface(u.ux,u.uy));
	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		/* Monsters which swim also happen not to be able to dig */
		You("cannot stay under%s long enough.",
				is_pool(u.ux, u.uy) ? "water" : " the lava");
	} else if (digtyp == 2) {
		/* axes can't cut through floor */
		pline_The("floor is too hard to chop through.");		
	} else {
		if (digging.pos.x != u.ux || digging.pos.y != u.uy ||
			!on_level(&digging.level, &u.uz) || !digging.down) {
		    digging.chew = FALSE;
		    digging.down = TRUE;
		    digging.pos.x = u.ux;
		    digging.pos.y = u.uy;
		    assign_level(&digging.level, &u.uz);
		    digging.effort = 0;
		    You("start %s downward.",
    		    	(digtyp == 1 ? "cutting" : "digging"));
		    if (*u.ushops) shopdig(0);
		} else
		    You("continue %s downward.", 
		    	(digtyp == 1 ? "cutting" : "digging"));
		did_dig_msg = FALSE;
		if (digtyp == 1) /* can't use axe downwards */
		   set_occupation(dig, "cutting", 0);
		else set_occupation(dig, "digging", 0);
	}
	return(1);
}

#endif /* OVLB */
#ifdef OVL0

/* Return TRUE if monster died, FALSE otherwise.  Called from m_move(). */
boolean
mdig_tunnel(mtmp)
register struct monst *mtmp;
{
	register struct rm *here;
	int pile = rnd(12);

	here = &levl[mtmp->mx][mtmp->my];
 	if (here->typ == SDOOR)
 	    cvt_sdoor_to_door(here);	/* ->typ = DOOR */
	if (IS_TREE(here->typ))
		/* KMH -- Trees shouldn't create piles */
		pile = 0;

	/* Eats away door if present & closed or locked */
	if (closed_door(mtmp->mx, mtmp->my)) {
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    unblock_point(mtmp->mx, mtmp->my);  /* vision */
	    if (here->doormask & D_TRAPPED) {
		here->doormask = D_NODOOR;
		if (mb_trapped(mtmp)) { /* mtmp is killed */
		    newsym(mtmp->mx, mtmp->my);
		    return TRUE;
		}
	    } else {
		if (!rn2(3) && flags.verbose)   /* not too often.. */
		    You_feel("an unexpected draft.");
		here->doormask = D_BROKEN;
	    }
	    newsym(mtmp->mx, mtmp->my);
	    return FALSE;
	} else
	if (!IS_ROCK(here->typ)) /* no dig */
	    return FALSE;

	/* Only rock and walls fall through to this point. */
	if ((here->wall_info & W_NONDIGGABLE) != 0) {
	    impossible("mdig_tunnel:  %s at (%d,%d) is undiggable",
		       (IS_WALL(here->typ) ? "wall" : "stone"),
		       (int) mtmp->mx, (int) mtmp->my);
	    return FALSE;       /* still alive */
	}

	if (IS_WALL(here->typ)) {
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
	    if (flags.soundok && flags.verbose && !rn2(5))
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
		You_hear("crashing rock.");
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    if (level.flags.is_maze_lev) {
		here->typ = ROOM;
	    } else if (level.flags.is_cavernous_lev) {
		here->typ = CORR;
	    } else {
		here->typ = DOOR;
		here->doormask = D_NODOOR;
	    }
	} else
	    /* KMH -- Added support for trees */
	    here->typ = level.flags.arboreal ? ROOM : CORR;

	if (pile && pile < 5)   /* leave behind some rocks? */
	    (void) mksobj_at((pile == 1) ? BOULDER : ROCK,
			     mtmp->mx, mtmp->my, TRUE, FALSE);
	newsym(mtmp->mx, mtmp->my);
	if (!sobj_at(BOULDER, mtmp->mx, mtmp->my))
	    unblock_point(mtmp->mx, mtmp->my);  /* vision */

	return FALSE;
}

#endif /* OVL0 */
#ifdef OVL3

/* digging via wand zap or spell cast */
void
zap_dig()
{
	struct rm *room;
	struct monst *mtmp;
/*        struct obj *otmp;*/
        register struct obj *otmp, *next_obj;
	int zx, zy, digdepth;
	boolean shopdoor, shopwall, maze_dig;
	/*
	 * Original effect (approximately):
	 * from CORR: dig until we pierce a wall
	 * from ROOM: pierce wall and dig until we reach
	 * an ACCESSIBLE place.
	 * Currently: dig for digdepth positions;
	 * also down on request of Lennart Augustsson.
	 */

	if (u.uswallow) {
	    mtmp = u.ustuck;

	    if (!is_whirly(mtmp->data)) {
		if (is_animal(mtmp->data))
		    You("pierce %s stomach wall!", s_suffix(mon_nam(mtmp)));
		mtmp->mhp = 1;          /* almost dead */
		expels(mtmp, mtmp->data, !is_animal(mtmp->data));
	    }
	    return;
	} /* swallowed */

	if (u.dz) {
	    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !Underwater) {
		if (u.dz < 0 || On_stairs(u.ux, u.uy)) {
		    if (On_stairs(u.ux, u.uy))
			pline_The("beam bounces off the %s and hits the %s.",
			      (u.ux == xdnladder || u.ux == xupladder) ?
			      "ladder" : "stairs", ceiling(u.ux, u.uy));
		    You("loosen a rock from the %s.", ceiling(u.ux, u.uy));
		    pline("It falls on your %s!", body_part(HEAD));
		    losehp(rnd((uarmh && is_metallic(uarmh)) ? 2 : 6),
			   "falling rock", KILLED_BY_AN);
		    if ((otmp = mksobj_at(ROCK, u.ux, u.uy, FALSE, FALSE)) != 0) {
			(void)xname(otmp);      /* set dknown, maybe bknown */
			stackobj(otmp);
		    }
		    if (Invisible) newsym(u.ux, u.uy);
		} else {
		    (void) dighole(FALSE);
		}
	    }
	    return;
	} /* up or down */

	/* normal case: digging across the level */
	shopdoor = shopwall = FALSE;
	maze_dig = level.flags.is_maze_lev && !Is_earthlevel(&u.uz);
	zx = u.ux + u.dx;
	zy = u.uy + u.dy;
	digdepth = rn1(18, 8);
	tmp_at(DISP_BEAM, cmap_to_glyph(S_digbeam));
	while (--digdepth >= 0) {
	    if (!isok(zx,zy)) break;
	    room = &levl[zx][zy];
	    tmp_at(zx,zy);
	    delay_output();     /* wait a little bit */

            /* WAC check for monster, boulder */
            if ((mtmp = m_at(zx, zy)) != 0) {
                if (made_of_rock(mtmp->data)) {
                    You("gouge a hole in %s!", mon_nam(mtmp));
                    mtmp->mhp /= 2;
                    if (mtmp->mhp < 1) mtmp->mhp = 1;
                } else pline("%s is unaffected!", Monnam(mtmp));
            }
            for(otmp = level.objects[zx][zy]; otmp; otmp = next_obj) {
                next_obj = otmp->nexthere;
		/* vaporize boulders */
                if (otmp->otyp == BOULDER) {
		    delobj(otmp);
		    /* A little Sokoban guilt... */
		    if (In_sokoban(&u.uz))
			change_luck(-1);
		    unblock_point(zx, zy);
		    newsym(zx, zy);
		    pline_The("boulder is vaporized!");
		}
		break;
            }

            if (closed_door(zx, zy) || room->typ == SDOOR) {
#ifdef DEVEL_BRANCH
		/* ALI - Artifact doors */
		if (artifact_door(zx, zy)) {
		    if (cansee(zx, zy))
			pline_The("door glows then fades.");
		    break;
		}
#endif
		if (*in_rooms(zx,zy,SHOPBASE)) {
		    add_damage(zx, zy, 400L);
		    shopdoor = TRUE;
		}
		if (room->typ == SDOOR)
		    room->typ = DOOR;
		else if (cansee(zx, zy))
		    pline_The("door is razed!");
		room->doormask = D_NODOOR;
		unblock_point(zx,zy); /* vision */
		digdepth -= 2;
		if (maze_dig) break;
	    } else if (maze_dig) {
		if (IS_WALL(room->typ)) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			if (*in_rooms(zx,zy,SHOPBASE)) {
			    add_damage(zx, zy, 200L);
			    shopwall = TRUE;
			}
			room->typ = ROOM;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The("wall glows then fades.");
		    break;
		} else if (room->typ == STONE || room->typ == SCORR) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			room->typ = CORR;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The("rock glows then fades.");
		    break;
		}
	    } else if (IS_ROCK(room->typ)) {
		if (!may_dig(zx,zy)) break;
		if (IS_WALL(room->typ) || room->typ == SDOOR) {
		    if (*in_rooms(zx,zy,SHOPBASE)) {
			add_damage(zx, zy, 200L);
			shopwall = TRUE;
		    }
		    if (level.flags.is_cavernous_lev) {
			room->typ = CORR;
		    } else {
			room->typ = DOOR;
			room->doormask = D_NODOOR;
		    }
		    digdepth -= 2;
		} else {        /* IS_ROCK but not IS_WALL or SDOOR */
		    room->typ = CORR;
		    digdepth--;
		}
		unblock_point(zx,zy); /* vision */
	    }
	    zx += u.dx;
	    zy += u.dy;
	} /* while */
	tmp_at(DISP_END,0);     /* closing call */
	if (shopdoor || shopwall)
	    pay_for_damage(shopdoor ? "destroy" : "dig into");
	return;
}

/* move objects from fobj/nexthere lists to buriedobjlist, keeping position */
/* information */
struct obj *
bury_an_obj(otmp)
	struct obj *otmp;
{
	struct obj *otmp2;
	boolean under_ice;

#ifdef DEBUG
	pline("bury_an_obj: %s", xname(otmp));
#endif
	if (otmp == uball)
		unpunish();
	/* after unpunish(), or might get deallocated chain */
	otmp2 = otmp->nexthere;
	/*
	 * obj_resists(,0,0) prevents Rider corpses from being buried.
	 * It also prevents The Amulet and invocation tools from being
	 * buried.  Since they can't be confined to bags and statues,
	 * it makes sense that they can't be buried either, even though
	 * the real reason there (direct accessibility when carried) is
	 * completely different.
	 */
	if (otmp == uchain || obj_resists(otmp, 0, 0))
		return(otmp2);

	if (otmp->otyp == LEASH && otmp->leashmon != 0)
		o_unleash(otmp);

#ifdef STEED
	if (otmp == usaddle)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	if (otmp->lamplit && otmp->otyp != POT_OIL)
		end_burn(otmp, TRUE);

	obj_extract_self(otmp);

	under_ice = is_ice(otmp->ox, otmp->oy);
	if (otmp->otyp == ROCK && !under_ice) {
		/* merges into burying material */
		obfree(otmp, (struct obj *)0);
		return(otmp2);
	}
	/*
	 * Start a rot on organic material.  Not corpses -- they
	 * are already handled.
	 */
	if (otmp->otyp == CORPSE) {
	    ;           /* should cancel timer if under_ice */
	} else if ((under_ice ? otmp->oclass == POTION_CLASS : is_organic(otmp))
		&& !obj_resists(otmp, 5, 95)) {
	    (void) start_timer((under_ice ? 0L : 250L) + (long)rnd(250),
			       TIMER_OBJECT, ROT_ORGANIC, (genericptr_t)otmp);
	}
	add_to_buried(otmp);
	return(otmp2);
}

void
bury_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	if(level.objects[x][y] != (struct obj *)0)
		pline("bury_objs: at %d, %d", x, y);
#endif
	for (otmp = level.objects[x][y]; otmp; otmp = otmp2)
		otmp2 = bury_an_obj(otmp);

	/* don't expect any engravings here, but just in case */
	del_engr_at(x, y);
	newsym(x, y);
}

/* move objects from buriedobjlist to fobj/nexthere lists */
void
unearth_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	pline("unearth_objs: at %d, %d", x, y);
#endif
	for (otmp = level.buriedobjlist; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (otmp->ox == x && otmp->oy == y) {
		    obj_extract_self(otmp);
		    if (otmp->timed)
			(void) stop_timer(ROT_ORGANIC, (genericptr_t)otmp);
		    place_object(otmp, x, y);
		    stackobj(otmp);
		}
	}
	del_engr_at(x, y);
	newsym(x, y);
}

/*
 * The organic material has rotted away while buried.  As an expansion,
 * we could add add partial damage.  A damage count is kept in the object
 * and every time we are called we increment the count and reschedule another
 * timeout.  Eventually the object rots away.
 *
 * This is used by buried objects other than corpses.  When a container rots
 * away, any contents become newly buried objects.
 */
/* ARGSUSED */
void
rot_organic(arg, timeout)
genericptr_t arg;
long timeout;   /* unused */
{
#if defined(MAC_MPW)
# pragma unused ( timeout )
#endif
	struct obj *obj = (struct obj *) arg;

	while (Has_contents(obj)) {
	    /* We don't need to place contained object on the floor
	       first, but we do need to update its map coordinates. */
	    obj->cobj->ox = obj->ox,  obj->cobj->oy = obj->oy;
	    /* Everything which can be held in a container can also be
	       buried, so bury_an_obj's use of obj_extract_self insures
	       that Has_contents(obj) will eventually become false. */
	    (void)bury_an_obj(obj->cobj);
	}
	obj_extract_self(obj);
	obfree(obj, (struct obj *) 0);
}

/*
 * Called when a corpse has rotted completely away.
 */
void
rot_corpse(arg, timeout)
genericptr_t arg;
long timeout;   /* unused */
{
	xchar x = 0, y = 0;
	struct obj *obj = (struct obj *) arg;
	boolean on_floor = obj->where == OBJ_FLOOR,
                in_minvent = obj->where == OBJ_MINVENT,
		in_invent = obj->where == OBJ_INVENT;

	if (on_floor) {
	    x = obj->ox;
	    y = obj->oy;
        } else if (in_minvent) {
            /* WAC unwield if wielded */
            if (MON_WEP(obj->ocarry) && MON_WEP(obj->ocarry) == obj) {
		    obj->owornmask &= ~W_WEP;
                    MON_NOWEP(obj->ocarry);
	    }
	} else if (in_invent) {
	    if (flags.verbose)
		Your("%s%s rot%s away%c",
		     obj == uwep ? "wielded " : "", corpse_xname(obj, FALSE),
		     obj->quan == 1L ? "s" : "", obj == uwep ? '!' : '.');
	    if (obj == uwep) {
			uwepgone();     /* now bare handed */
			stop_occupation();
		} else if (obj == uswapwep) {
			uswapwepgone();
			stop_occupation();
	    } else if (obj == uquiver) {
			uqwepgone();
			stop_occupation();
	    } else if (obj == uswapwep) {
		uswapwepgone();
		stop_occupation();
	    } else if (obj == uquiver) {
		uqwepgone();
		stop_occupation();
	    }
	} else if (obj->where == OBJ_MINVENT && obj->owornmask) {
	    if (obj == MON_WEP(obj->ocarry)) {
		obj->owornmask &= ~W_WEP;
		MON_NOWEP(obj->ocarry);
		}
	}
	rot_organic(arg, timeout);
	if (on_floor) newsym(x, y);
	else if (in_invent) update_inventory();
}

#if 0
void
bury_monst(mtmp)
struct monst *mtmp;
{
#ifdef DEBUG
	pline("bury_monst: %s", mon_nam(mtmp));
#endif
	if(canseemon(mtmp)) {
	    if(is_flyer(mtmp->data) || is_floater(mtmp->data)) {
		pline_The("%s opens up, but %s is not swallowed!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
		return;
	    } else
		pline_The("%s opens up and swallows %s!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
	}

	mtmp->mburied = TRUE;
	wakeup(mtmp);                   /* at least give it a chance :-) */
	newsym(mtmp->mx, mtmp->my);
}

void
bury_you()
{
#ifdef DEBUG
	pline("bury_you");
#endif
	/* KMH, balance patch -- new intrinsic */
    if (!Levitation && !Flying) {
	if(u.uswallow)
	    You_feel("a sensation like falling into a trap!");
	else
	    pline_The("%s opens beneath you and you fall in!",
		  surface(u.ux, u.uy));

	u.uburied = TRUE;
	if(!Strangled && !Breathless) Strangled = 6;
	under_ground(1);
    }
}

void
unearth_you()
{
#ifdef DEBUG
	pline("unearth_you");
#endif
	u.uburied = FALSE;
	under_ground(0);
	if(!uamul || uamul->otyp != AMULET_OF_STRANGULATION)
		Strangled = 0;
	vision_recalc(0);
}

void
escape_tomb()
{
#ifdef DEBUG
	pline("escape_tomb");
#endif
	if ((Teleportation || can_teleport(youmonst.data)) &&
	    (Teleport_control || rn2(3) < Luck+2)) {
		You("attempt a teleport spell.");
		(void) dotele();        /* calls unearth_you() */
	} else if(u.uburied) { /* still buried after 'port attempt */
		boolean good;

		if(amorphous(youmonst.data) || Passes_walls ||
		   noncorporeal(youmonst.data) || unsolid(youmonst.data) ||
		   (tunnels(youmonst.data) && !needspick(youmonst.data))) {

		    You("%s up through the %s.",
			(tunnels(youmonst.data) && !needspick(youmonst.data)) ?
			 "try to tunnel" : (amorphous(youmonst.data)) ?
			 "ooze" : "phase", surface(u.ux, u.uy));

		    if(tunnels(youmonst.data) && !needspick(youmonst.data))
			good = dighole(TRUE);
		    else good = TRUE;
		    if(good) unearth_you();
		}
	}
}

void
bury_obj(otmp)
struct obj *otmp;
{

#ifdef DEBUG
	pline("bury_obj");
#endif
	if(cansee(otmp->ox, otmp->oy))
	   pline_The("objects on the %s tumble into a hole!",
		surface(otmp->ox, otmp->oy));

	bury_objs(otmp->ox, otmp->oy);
}
#endif

#ifdef DEBUG
void
wiz_debug_cmd() /* in this case, bury everything at your loc and around */
{
	int x, y;

	for (x = u.ux - 1; x <= u.ux + 1; x++)
	    for (y = u.uy - 1; y <= u.uy + 1; y++)
		if (isok(x,y)) bury_objs(x,y);
}

#endif /* DEBUG */
#endif /* OVL3 */

/*dig.c*/