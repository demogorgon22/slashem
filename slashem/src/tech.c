/*      SCCS Id: @(#)tech.c    3.2     98/Oct/30        */
/*      Original Code by Warren Cheung (Basis: spell.c, attrib.c) */
/*      Copyright (c) M. Stephenson 1988                          */
/* NetHack may be freely redistributed.  See license for details. */

/* All of the techs from cmd.c are ported here */

#include "hack.h"

static boolean FDECL(gettech, (int *));
static boolean FDECL(dotechmenu, (int, int *));
static void NDECL(doblitzlist);
static int FDECL(get_tech_no,(int));
static int FDECL(techeffects, (int));
static void FDECL(hurtmon, (struct monst *,int));
static int FDECL(mon_to_zombie, (int));
STATIC_PTR int NDECL(tinker);
STATIC_PTR int NDECL(draw_energy);
static const struct innate_tech * NDECL(role_tech);
static const struct innate_tech * NDECL(race_tech);
static int NDECL(doblitz);
static int NDECL(blitz_chi_strike);
static int NDECL(blitz_e_fist);
static int NDECL(blitz_pummel);
static int NDECL(blitz_g_slam);
static int NDECL(blitz_dash);
static int NDECL(blitz_power_surge);
static int NDECL(blitz_spirit_bomb);
static void FDECL(dash, (int, int, int, BOOLEAN_P));

static NEARDATA schar delay;            /* moves left for tinker/energy draw */

/* 
 * Do try to keep the names <= 25 chars long, or else the
 * menu will look bad :B  WAC
 */
 
STATIC_OVL NEARDATA const char *tech_names[] = {
	"no technique",
	"berserk",
	"kiii",
	"research",
	"surgery",
	"reinforce memory",
	"missile flurry",
	"weapon practice",
	"eviscerate",
	"healing hands",
	"calm steed",
	"turn undead",
	"vanish",
	"cutthroat",
	"blessing",
	"elemental fist",
	"primal roar",
	"liquid leap",
	"critical strike",
	"sigil of control",
	"sigil of tempest",
	"sigil of discharge",
	"raise zombies",
	"revivification",
	"ward against flame",
	"ward against ice",
	"ward against lightning",
	"tinker",
	"rage eruption",
	"blink",
	"chi strike",
	"draw energy",
	"chi healing",
	"disarm",
	"dazzle",
	"chained blitz",
	"pummel",
	"ground slam",
	"air dash",
	"power surge",
	"spirit bomb",
	""
};

static const struct innate_tech 
	/* Roles */
	arc_tech[] = { {   1, T_RESEARCH, 1},
		       {   0, 0, 0} },
	bar_tech[] = { {   1, T_BERSERK, 1},
		       {   0, 0, 0} },
	cav_tech[] = { {   1, T_PRIMAL_ROAR, 1},
		       {   0, 0, 0} },
	fla_tech[] = { {   1, T_REINFORCE, 1},
		       {   3, T_POWER_SURGE, 1},
		       {   5, T_DRAW_ENERGY, 1},
		       {  10, T_SIGIL_TEMPEST, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },
	hea_tech[] = { {   1, T_SURGERY, 1},
		       {  20, T_REVIVE, 1},
		       {   0, 0, 0} },
	ice_tech[] = { {   1, T_REINFORCE, 1},
		       {   5, T_DRAW_ENERGY, 1},
		       {  10, T_SIGIL_TEMPEST, 1},
		       {  12, T_POWER_SURGE, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },
	kni_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_HEAL_HANDS, 1},
		       {   0, 0, 0} },
	mon_tech[] = { {   1, T_PUMMEL, 1},
		       {   1, T_DASH, 1},
		       {   1, T_BLITZ, 1},
		       {   2, T_CHI_STRIKE, 1},
	  	       {   4, T_CHI_HEALING, 1},
	  	       {   6, T_E_FIST, 1},
		       {   8, T_DRAW_ENERGY, 1},
		       {  10, T_G_SLAM, 1},
		       {  11, T_WARD_FIRE, 1},
		       {  13, T_WARD_COLD, 1},
		       {  15, T_WARD_ELEC, 1},
		       {  17, T_SPIRIT_BOMB, 1},
		       {  20, T_POWER_SURGE, 1},
		       {   0, 0, 0} },
	nec_tech[] = { {   1, T_REINFORCE, 1},
		       {   1, T_RAISE_ZOMBIES, 1},
		       {  10, T_POWER_SURGE, 1},
		       {  15, T_SIGIL_TEMPEST, 1},
		       {   0, 0, 0} },
	pri_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_BLESSING, 1},
		       {   0, 0, 0} },
	ran_tech[] = { {   1, T_FLURRY, 1},
		       {   0, 0, 0} },
	rog_tech[] = { {   1, T_CRIT_STRIKE, 1},
		       {  15, T_CUTTHROAT, 1},
		       {   0, 0, 0} },
	sam_tech[] = { {   1, T_KIII, 1},
		       {   0, 0, 0} },
	tou_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	und_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_PRACTICE, 1},
		       {   0, 0, 0} },
	val_tech[] = { {   1, T_PRACTICE, 1},
		       {   0, 0, 0} },
#ifdef YEOMAN
	yeo_tech[] = {
#ifdef STEED
		       {   1, T_CALM_STEED, 1},
#endif
		       {   0, 0, 0} },
#endif
	wiz_tech[] = { {   1, T_REINFORCE, 1},
		       {   3, T_DRAW_ENERGY, 1},
		       {   5, T_POWER_SURGE, 1},
		       {   7, T_SIGIL_CONTROL, 1},
		       {  14, T_SIGIL_TEMPEST, 1},
		       {  20, T_SIGIL_DISCHARGE, 1},
		       {   0, 0, 0} },		       
	/* Races */
	dop_tech[] = { {   1, T_LIQUID_LEAP, 1},
		       {   0, 0, 0} },
	dwa_tech[] = { {   1, T_RAGE, 1},
		       {   0, 0, 0} },
	elf_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	gno_tech[] = { {   1, T_VANISH, 1},
		       {   7, T_TINKER, 1},
		       {   0, 0, 0} },
	hob_tech[] = { {   1, T_BLINK, 1},
		       {   0, 0, 0} },
	lyc_tech[] = { {   1, T_EVISCERATE, 1},
		       {  10, T_BERSERK, 1},
		       {   0, 0, 0} },
	vam_tech[] = { {   1, T_DAZZLE, 1},
		       {   0, 0, 0} };
	/* Orc */

/* Local Macros 
 * these give you direct access to the player's list of techs.  
 * Are you sure you don't want to use tech_inuse,  which is the
 * extern function for checking whether a fcn is inuse
 */

#define techt_inuse(tech)       tech_list[tech].t_inuse
#define techtout(tech)        tech_list[tech].t_tout
#define techlev(tech)         (u.ulevel - tech_list[tech].t_lev)
#define techid(tech)          tech_list[tech].t_id
#define techname(tech)        (tech_names[techid(tech)])
#define techlet(tech)  \
        ((char)((tech < 26) ? ('a' + tech) : ('A' + tech - 26)))

/* Whether you know the tech */
boolean
tech_known(tech)
	short tech;
{
	int i;
	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) 
		     return TRUE;
	}
	return FALSE;
}
/* Called to teach a new tech.  Level is starting tech level */
void
learntech(tech, level)
	short tech;
	int level;
{
	int i;

	for (i = 0; i < MAXTECH; i++) {
	    if (level > 0) {
		if (techid(i) == tech) {
		     pline ("Error:  Tech already known.");
		     return;
		} else if (techid(i) == NO_TECH)  {
		    tech_list[i].t_id = tech;
		    tech_list[i].t_lev = (u.ulevel ? u.ulevel - level : 0);
		    tech_list[i].t_tout = 0; /* Can use immediately*/
                    tech_list[i].t_inuse = 0; /* not in use */
		    break;
		}
	    } else if (level < 0) {
		if (techid(i) == tech) {
		    tech_list[i].t_id = NO_TECH;
		    return;
		} else if (i == MAXTECH) break;
	    } else {
	   	impossible ("Invalid Tech Level!");
	   	return;
	    }
	}
	if (i == MAXTECH) impossible("Reached end of Tech list!");
	return;
}

/*
 * Return TRUE if a tech was picked, with the tech index in the return
 * parameter.  Otherwise return FALSE.
 */
static boolean
gettech(tech_no)
        int *tech_no;
{
        int ntechs, idx;
	char ilet, lets[BUFSZ], qbuf[QBUFSZ];

        if (techid(0) == NO_TECH)  {
            You("don't know any techniques right now.");
	    return FALSE;
	}
	if (flags.menu_style == MENU_TRADITIONAL) {
            /* we know there is at least 1 known tech */
            for (ntechs = 1; ntechs < MAXTECH
                            && techid(ntechs) != NO_TECH; ntechs++)
		continue;

            if (ntechs == 1)  Strcpy(lets, "a");
            else if (ntechs < 27)  Sprintf(lets, "a-%c", 'a' + ntechs - 1);
            else if (ntechs == 27)  Sprintf(lets, "a-z A");
            else Sprintf(lets, "a-z A-%c", 'A' + ntechs - 27);

	    for(;;)  {
                Sprintf(qbuf, "Perform which technique? [%s ?]", lets);
		if ((ilet = yn_function(qbuf, (char *)0, '\0')) == '?')
		    break;

		if (index(quitchars, ilet))
		    return FALSE;

		if (letter(ilet) && ilet != '@') {
		    /* in a-zA-Z, convert back to an index */
		    if (lowc(ilet) == ilet)     /* lower case */
			idx = ilet - 'a';
		    else
			idx = ilet - 'A' + 26;

                    if (idx < ntechs) {
                        *tech_no = idx;
			return TRUE;
		    }
		}
                You("don't know that technique.");
	    }
	}
        return dotechmenu(PICK_ONE, tech_no);
}

static boolean
dotechmenu(how, tech_no)
	int how;
        int *tech_no;
{
	winid tmpwin;
	int i, n;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

        Sprintf(buf, "%-30s Level   Status", "Name");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

        for (i = 0; i < MAXTECH && techid(i) != NO_TECH; i++) {
                Sprintf(buf, "%-26s %5d   %s",
                    techname(i), techlev(i),
                      tech_inuse(techid(i)) ? "Active" :
                      !techtout(i) ? "Prepared" : 
		      techtout(i) > 100 ? "Not Ready" : 
		      "Soon");

		any.a_int = i+1;        /* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
                         techlet(i), 0, ATR_NONE, buf, MENU_UNSELECTED);
	}
        end_menu(tmpwin, how == PICK_ONE ? "Choose a technique" :
                                           "Currently known techniques");

	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
                *tech_no = selected[0].item.a_int - 1;
		free((genericptr_t)selected);
		return TRUE;
	}
	return FALSE;
}

static int
get_tech_no(tech)
int tech;
{
	int i;

	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) {
			return(i);
		}
	}
	return (-1);
}

int
dotech()
{
	int tech_no;

	if (gettech(&tech_no))
	    return techeffects(tech_no);
	return 0;
}


/* gettech is reworked getspell */
/* reworked class special effects code */
/* adapted from specialpower in cmd.c */
static int
techeffects(tech_no)
int tech_no;
{
	/* These variables are used in various techs */
	struct obj *obj;
	const char *str;
	struct monst *mtmp;
	int num;
	char Your_buf[BUFSZ];
	char allowall[2];
	int i, j;


	/* check timeout */
	if (tech_inuse(techid(tech_no))) {
	    pline("This technique is already active!");
	    return (0);
	}
        if (techtout(tech_no)) {
	    You("have to wait %s before using your technique again.",
                (techtout(tech_no) > 100) ?
                        "for a while" : "a little longer");
#ifdef WIZARD
            if (!wizard || (yn("Use technique anyways?") == 'n'))
#endif
                return(0);
        }

/* switch to the tech and do stuff */
        switch (techid(tech_no)) {
            case T_RESEARCH:
		/* WAC stolen from the spellcasters...'A' can identify from
        	   historical research*/
		if(Hallucination || Stunned || Confusion) {
		    You("can't concentrate right now!");
		    return(0);
		} else if((ACURR(A_INT) + ACURR(A_WIS)) < rnd(60)) {
			pline("Nothing in your pack looks familiar.");
                    techtout(tech_no) = rn1(500,500);
		    break;
		} else if(invent) {
			You("examine your possessions.");
			identify_pack((int) ((techlev(tech_no) / 10) + 1));
		} else {
			/* KMH -- fixed non-compliant string */
		    You("are already quite familiar with the contents of your pack.");
		    break;
		}
                techtout(tech_no) = rn1(500,1500);
		break;
            case T_EVISCERATE:
		/*only when empty handed, in human form!*/
		if (Upolyd) {
		        You("can't do this while polymorphed!");
		        break;
		}
		if (!uwep) {
		        Your("fingernails extend into claws!");
		        aggravate();
                        techt_inuse(tech_no) = d(2,4) + (techlev(tech_no)/5) + 2; /* [max] was d(2,8) */
                        techtout(tech_no) = rn1(1000,1000); /* [max] increased delay */
		} else {
		    You("can't do this while holding a weapon!");
		    return(0);
		}
		break;
            case T_BERSERK:
			You("fly into a berserk rage!");
               		techt_inuse(tech_no) = d(2,8) +
               		(techlev(tech_no)/5) + 2;
			incr_itimeout(&HFast, techt_inuse(tech_no));
			techtout(tech_no) = rn1(1000,500);
			return(0);
			break;
            case T_REINFORCE:
			/* WAC spell-users can study their known spells*/
			if(Hallucination || Stunned || Confusion) {
				You("can't concentrate right now!");
				break;
               		} else {
				You("concentrate...");
				if (studyspell()) techtout(tech_no) = rn1(1000,500); /*in spell.c*/
			}
               break;
            case T_FLURRY:
                Your("%s %s become blurs as they reach for your quiver!",
			uarmg ? "gloved" : "bare",      /* Del Lamb */
			makeplural(body_part(HAND)));
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
                techtout(tech_no) = rn1(1000,500);
		break;
            case T_PRACTICE:
                if(!uwep || (weapon_type(uwep) == P_NONE)) {
		    You("are not wielding a weapon!");
		    return(0);
		} else if(uwep->known == TRUE) {
                    practice_weapon();
		} else {
                    if (not_fully_identified(uwep)) {
                        You("examine %s.", doname(uwep));
                            if (rnd(15) <= ACURR(A_INT)) {
                                makeknown(uwep->otyp);
                                uwep->known = TRUE;
                                You("discover it is %s",doname(uwep));
                                } else
                     pline("Unfortunately, you didn't learn anything new.");
                    } 
                /*WAC Added practicing code - in weapon.c*/
                    practice_weapon();
		}
                techtout(tech_no) = rn1(500,500);
		break;
            case T_SURGERY:
		if (Hallucination || Stunned || Confusion) {
		    You("are in no condition to perform surgery!");
		    break;
		}
		if ((Sick) || (Slimed)) {       /* WAC cure sliming too */
		    if(carrying(SCALPEL)) {
			pline("Using your scalpel (ow!), you cure your infection!");
			make_sick(0L,(char *)0, TRUE,SICK_ALL);
			Slimed = 0;
			if(u.uhp > 6) u.uhp -= 5;
			else          u.uhp = 1;
                        techtout(tech_no) = rn1(500,500);
			break;
		    } else pline("If only you had a scalpel...");
		}
		if (u.uhp < u.uhpmax) {
		    if(carrying(MEDICAL_KIT)) {
			pline("Using your medical kit, you bandage your wounds.");
			u.uhp += (techlev(tech_no) * (rnd(2)+1)) + rn1(5,5);
		    } else {
			pline("You bandage your wounds as best you can.");
			u.uhp += (techlev(tech_no)) + rn1(5,5);
		    }
                    techtout(tech_no) = rn1(1000,500);
		    if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
		} else pline("You don't need your healing powers!");
		break;
            case T_HEAL_HANDS:
		if (u.uhp < u.uhpmax || Sick || Slimed) { /*WAC heal sliming */
			if (Sick) You("lay your hands on the foul sickness...");
			pline("A warm glow spreads through your body!");
			if (Slimed) pline_The("slime is removed.");
			Slimed = 0;
			if(Sick) make_sick(0L,(char*)0, TRUE, SICK_ALL);
			else     u.uhp += (techlev(tech_no) * 4);
			if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
                        techtout(tech_no) = 3000;
		} else pline("Nothing happens...");
		break;
            case T_KIII:
		You("scream \"KIIILLL!\"");
		aggravate();
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
                techtout(tech_no) = rn1(1000,500);
		return(0);
		break;
#ifdef STEED
	    case T_CALM_STEED:
                if (u.usteed) {
                        pline("%s gets tamer.", Monnam(u.usteed));
                        tamedog(u.usteed, (struct obj *) 0);
                        techtout(tech_no) = rn1(1000,500);
                } else
                        Your("technique is only effective when riding a monster.");
                break;
#endif
            case T_TURN_UNDEAD:
                return(turn_undead());
	    case T_VANISH:
		if (Invisible && Fast) {
			You("are already quite nimble and undetectable.");
		}
                techt_inuse(tech_no) = rn1(50,50) + techlev(tech_no);
		if (!Invisible) pline("In a puff of smoke,  you disappear!");
		if (!Fast) You("feel more nimble!");
		incr_itimeout(&HInvis, techt_inuse(tech_no));
		incr_itimeout(&HFast, techt_inuse(tech_no));
		newsym(u.ux,u.uy);      /* update position */
		techtout(tech_no) = rn1(1000,500);
		break;
	    case T_CRIT_STRIKE:
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			You("decide against that idea.");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
			You("perform a flashy twirl!");
			return (0);
		} else {
			int oldhp = mtmp->mhp;
			int tmp = 0;
			
			if (!attack(mtmp)) return(0);
			if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp) {
			    tmp = oldhp - mtmp->mhp;
			    You("strike %s vital organs!", s_suffix(mon_nam(mtmp)));
			    if (humanoid(mtmp->data)) tmp = mtmp->mhp / 2;
			    else {
				You("are hampered by the differences in anatomy.");
				tmp = mtmp->mhp/4;
			    }
			    tmp += techlev(tech_no);
			    techtout(tech_no) = rn1(1000,500);
			    hurtmon(mtmp, tmp);
			}
		}
		break;
	    case T_CUTTHROAT:
		if (!is_blade(uwep)) {
			You("need a blade to perform cutthroat!");
			return (0);
		}
	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			pline("Things may be going badly,  but that's extreme.");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
			You("attack...nothing!");
			return (0);
		} else {
			int oldhp = mtmp->mhp;
			
			if (!attack(mtmp)) return(0);
			if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp) {
				int tmp = 0;
				if (!has_head(mtmp->data) || u.uswallow) {
					You("can't perform cutthroat on %s!",mon_nam(mtmp));
				}
				if (rn2(5) < (techlev(tech_no)/10 + 1)) {
					You("sever %s head!", s_suffix(mon_nam(mtmp)));
					tmp = mtmp->mhp;
				} else {
					You("hurt %s badly!", s_suffix(mon_nam(mtmp)));
					tmp = mtmp->mhp / 2;
				}
				tmp += techlev(tech_no);
				techtout(tech_no) = rn1(1000,500);
				hurtmon(mtmp, tmp);
			}
		}
		break;
	    case T_BLESSING:
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		
		if ( !(obj = getobj(allowall, "bless"))) return(0);
		pline("An aura of holiness surrounds your hands!");
                if (!Blind) (void) Shk_Your(Your_buf, obj);
		if (obj->cursed) {
                	if (!Blind)
                    		pline("%s %s %s.",Your_buf,
						  aobjnam(obj, "softly glow"),
						  hcolor(amber));
				uncurse(obj);
				obj->bknown=1;
		} else if(!obj->blessed) {
			if (!Blind) {
				str = hcolor(light_blue);
				pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  index(vowels, *str) ? "n" : "", str);
			}
			bless(obj);
			obj->bknown=1;
		} else {
			if (obj->bknown) {
				pline ("That object is already blessed!");
				return(0);
			}
			obj->bknown=1;
			pline("The aura fades.");
		}
		techtout(tech_no) = rn1(1000,500);
		techtout(tech_no) -= (techlev(tech_no)*10);
		break;
	    case T_E_FIST: 
	    	blitz_e_fist();
#if 0
		str = makeplural(body_part(HAND));
                You("focus the powers of the elements into your %s", str);
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/3 + 1)) + d(1,4) + 2;
#endif
		techtout(tech_no) = rn1(1000,500);
	    	break;
	    case T_PRIMAL_ROAR:	    	
	    	You("let out a bloodcurdling roar!");
	    	aggravate();

		techt_inuse(tech_no) = d(2,6) + (techlev(tech_no)) + 2;

		incr_itimeout(&HFast, techt_inuse(tech_no));

	    	for(i = -5; i <= 5; i++) for(j = -5; j <= 5; j++)
		    if(isok(u.ux+i, u.uy+j) && (mtmp = m_at(u.ux+i, u.uy+j))) {
		    	if (mtmp->mtame != 0 && !mtmp->isspell) {
		    	    struct permonst *ptr = mtmp->data;
		    	    int time = techt_inuse(tech_no);
		    	    int type = little_to_big(monsndx(ptr));
		    	    
		    	    ptr = &mons[type];

		    	    mtmp = tamedog(mtmp, (struct obj *) 0);

		    	    if (type) {
		    	    	mon_spec_poly(mtmp, ptr, time);
		    	    }
		    	}
		    }
		techtout(tech_no) = rn1(1000,500);
	    	break;
	    case T_LIQUID_LEAP: {
	    	coord cc;
	    	int dx, dy, sx, sy, range;

    		cc.x = sx = u.ux;
		cc.y = sy = u.uy;

		getpos(&cc, TRUE, "the desired position");
		if(cc.x == -10) return 0; /* user pressed esc */

		dx = cc.x - u.ux;
		dy = cc.y - u.uy;
		/* allow diagonals */
	    	if ((dx && dy) && (dx != dy) && (dx != -dy)) {
	    		You("can only leap in straight lines!");
			return (0);
	    	} else if (distu(cc.x, cc.y) > (19 + techlev(tech_no))) {
	    		pline("Too far!");
			return (0);
		} else if (((mtmp = m_at(cc.x, cc.y)) != 0) ||
		    (!isok(cc.x, cc.y) ||
		    ((IS_ROCK(levl[cc.x][cc.y].typ) ||
		     sobj_at(BOULDER, cc.x, cc.y) || closed_door(cc.x, cc.y))))) {
			You("cannot flow there!"); /* MAR */
			return (0);
		} else {
		    You("liquify!");
		    if(u.utrap) {
			switch(u.utraptype) {
			    case TT_BEARTRAP: 
				You("slide out of the bear trap.");
				break;
			    case TT_PIT:
				You("leap from the pit!");
				break;
			    case TT_WEB:
				You("flow through the web!");
				break;
			    case TT_LAVA:
				You("separate from the lava!");
				u.utrap = 0;
				break;
			    case TT_INFLOOR:
				u.utrap = 0;
				You("ooze out of the floor!");
			}
			u.utrap = 0;
		    }
		    /* Fry the things in the path ;B */
		    if (dx) range = dx;
		    else range = dy;
		    if (range < 0) range = -range;
		    
		    dx = sgn(dx);
		    dy = sgn(dy);
		    
		    while (range-- > 0) {
		    	int tmp_invul = 0;
		    	
		    	if (!Invulnerable) Invulnerable = tmp_invul = 1;
			sx += dx; sy += dy;
			tmp_at(DISP_BEAM, zapdir_to_glyph(dx, dy, AD_ACID-1));
			tmp_at(sx,sy);
			delay_output(); /* wait a little */
		    	if ((mtmp = m_at(sx, sy)) != 0) {
			    int chance;
			    
			    chance = rn2(20);
		    	    if (!chance || (3 - chance) > AC_VALUE(find_mac(mtmp)))
		    	    	break;
		    	    You("catch %s in your acid trail!", mon_nam(mtmp));
		    	    if (!resists_acid(mtmp)) {
				int tmp = 1;
				/* Need to add a to-hit */
				tmp += d(2,4);
				tmp += rn2((int) (techlev(tech_no)/5 + 1));
				if (!Blind) pline_The("acid burns %s!", mon_nam(mtmp));
				hurtmon(mtmp, tmp);
			    } else if (!Blind) pline_The("acid doesn't affect %s!", mon_nam(mtmp));
			}
			/* Clean up */
			tmp_at(DISP_END,0);
			if (tmp_invul) Invulnerable = 0;
		    }

		    You("reform!");
		    teleds(cc.x, cc.y);
		    nomul(-1);
		    nomovemsg = "";
	    	}
		techtout(tech_no) = rn1(1000,500);
	    	break;
	    }
            case T_SIGIL_TEMPEST: 
		/* Have enough power? */
		num = 50 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("don't have enough power to invoke the sigil!");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("invoke the sigil of tempest!");
                techt_inuse(tech_no) = d(1,6) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_SIGIL_CONTROL:
		/* Have enough power? */
		num = 30 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("don't have enough power to invoke the sigil!");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("invoke the sigil of control!");
                techt_inuse(tech_no) = d(1,4) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_SIGIL_DISCHARGE:
		/* Have enough power? */
		num = 100 - techlev(tech_no)/5;
		if (u.uen < num) {
			You("don't have enough power to invoke the sigil!");
			return (0);
		}
		u.uen -= num;

		/* Invoke */
		You("invoke the sigil of discharge!");
                techt_inuse(tech_no) = d(1,4) + rnd(techlev(tech_no)/5 + 1) + 2;
		u_wipe_engr(2);
		return(0);
		break;
            case T_RAISE_ZOMBIES: {
            	int x,y;
            	
            	You("chant the ancient curse...");
		for (i=0; i<3; i++) for (j=0; j<3; j++) {
	            x = u.ux + i - 1; y = u.uy + j - 1;
			if (OBJ_AT(x,y)) {
				struct obj *obj, *obj2;

				for (obj = level.objects[x][y]; obj; obj = obj2) {
				    obj2 = obj->nexthere;

				    /* Only generate undead */
				    if (mon_to_zombie(obj->corpsenm) != -1) {
				    	obj->corpsenm = mon_to_zombie(obj->corpsenm);
				    	mtmp = revive(obj);
					if (mtmp) {
				    	    if (!resist(mtmp, SPBOOK_CLASS, 0, TELL)) {
			                       mtmp = tamedog(mtmp, (struct obj *) 0);
			                       You("dominate %s!", mon_nam(mtmp));
		                            } else setmangry(mtmp);
					}
				    }
				}
			}
		}
		nomul(-2); /* You need to recover */
		techtout(tech_no) = rn1(1000,500);
		techtout(tech_no) -= (techlev(tech_no)*10);
		break;
            }
            case T_REVIVE: 
            	
            	num = 100 - techlev(tech_no); /* WAC make this depend on mon? */
            	
            	if ((Upolyd && u.mh <= num) || (!Upolyd && u.uhp <= num)){
            		You("don't have the strength to perform revivification!");
            		return(0);
            	}

            	obj = floorfood("revive", 1);
            	if (!obj) return (0);
            	mtmp = revive(obj);
            	if (mtmp) (void) tamedog(mtmp, (struct obj *) 0);
            	if (Upolyd) u.mh -= num;
            	else u.uhp -= num;
		techtout(tech_no) = rn1(1000,500);
            	break;
	    case T_WARD_FIRE:
		/* Already have it intrinsically? */
		if (HFire_resistance & FROMOUTSIDE) return (0);

		You("invoke the ward against flame!");
		HFire_resistance += rn1(100,50);
		HFire_resistance += techlev(tech_no);
		techtout(tech_no) = rn1(1000,500);

	    	break;
	    case T_WARD_COLD:
		/* Already have it intrinsically? */
		if (HCold_resistance & FROMOUTSIDE) return (0);

		You("invoke the ward against ice!");
		HCold_resistance += rn1(100,50);
		HCold_resistance += techlev(tech_no);
		techtout(tech_no) = rn1(1000,500);

	    	break;
	    case T_WARD_ELEC:
		/* Already have it intrinsically? */
		if (HShock_resistance & FROMOUTSIDE) return (0);

		You("invoke the ward against lightning!");
		HShock_resistance += rn1(100,50);
		HShock_resistance += techlev(tech_no);
		techtout(tech_no) = rn1(1000,500);

	    	break;
	    case T_TINKER:
		if (Blind) {
			You("can't do any tinkering if you can't see!");
			return (0);
		}
		if (!uwep) {
			You("aren't holding an object to work on!");
			return (0);
		}
		You("are holding %s.", doname(uwep));
		if (yn("Start tinkering on this?") != 'y') return(0);
		You("start working on %s",doname(uwep));
		delay=-150 + techlev(tech_no);
		set_occupation(tinker, "tinkering", 0);
		break;
	    case T_RAGE:     	
		if (Upolyd) {
			You("cannot focus your anger!");
			return(0);
		}
	    	You("feel the anger inside you erupt!");
		num = 50 + (4 * techlev(tech_no));
	    	techt_inuse(tech_no) = num + 1;
		u.uhpmax += num;
		u.uhp = u.uhpmax;
		techtout(tech_no) = rn1(1000,500);
		break;	    
	    case T_BLINK:
	    	You("feel the flow of time slow to a crawl.");
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/10 + 1)) + 2;
		techtout(tech_no) = rn1(1000,500);	    
	    	break;
            case T_CHI_STRIKE:
            	if (!blitz_chi_strike()) return(0);
                techtout(tech_no) = rn1(1000,500);
		break;
            case T_DRAW_ENERGY:
            	if (u.uen == u.uenmax) {
            		if (Hallucination) You("are fully charged!");
			else You("cannot hold any more energy!");
			return(0);           		
            	}
                You("begin drawing energy from your surroundings!");
		delay=-15;
		set_occupation(draw_energy, "drawing energy", 0);                
                techtout(tech_no) = rn1(1000,500);
		break;
            case T_CHI_HEALING:
            	if (u.uen < 1) {
            		You("are too weak to attempt this!");
            		return(0);
            	}
		You("direct your internal energy to restoring your body!");
                techt_inuse(tech_no) = techlev(tech_no)*2 + 4;
                techtout(tech_no) = rn1(1000,500);
		break;	
	    case T_DISARM:
	    	if (P_SKILL(weapon_type(uwep)) == P_NONE) {
	    		You("aren't wielding a proper weapon!");
	    		return(0);
	    	}
	    	if ((P_SKILL(weapon_type(uwep)) < P_SKILLED) || (Blind)) {
	    		You("aren't capable of doing this!");
	    		return(0);
	    	}

	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			pline("Why don't you try wielding something else instead.");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp || mtmp->minvis) {
			You("don't see anything there!");
			return (0);
		}
	    	obj = MON_WEP(mtmp);   /* can be null */

	    	if (!obj) {
	    		You("can't disarm an unarmed foe!");
	    		return(0);
	    	}
		num = ((rn2(techlev(tech_no) + 15)) 
			* (P_SKILL(weapon_type(uwep)) - P_SKILLED + 1)) / 10;

		You("attempt to disarm %s...",mon_nam(mtmp));
		/* WAC can't yank out cursed items */
                if ((num && (!Fumbling || !rn2(10))) && !obj->cursed) {
		    obj_extract_self(obj);
		    possibly_unwield(mtmp);
		    obj->owornmask &= ~W_WEP;
		    switch(rn2(num + 1)) {
			case 2:
			    /* to floor near you */
			    You("knock %s %s to the %s!",
				s_suffix(mon_nam(mtmp)),
				xname(obj),
				surface(u.ux, u.uy));
			    if(obj->otyp == CRYSKNIFE)
				obj->otyp = WORM_TOOTH;
			    place_object(obj,u.ux, u.uy);
			    break;
			case 3:
			    /* right into your inventory */
			    if (rn2(25)) {
				You("snatch %s %s!",
					s_suffix(mon_nam(mtmp)),
					xname(obj));
					obj = hold_another_object(obj,
					   "You drop %s!", doname(obj),
					   (const char *)0);
			    /* proficient at disarming, but maybe not
			       so proficient at catching weapons */
			    }
#if 0
			    else {
				int hitu, hitvalu;

				hitvalu = 8 + obj->spe;
				hitu = thitu(hitvalu,
					dmgval(obj, &youmonst),
					obj, xname(obj));
				if (hitu) {
					You("The %s hits you as you try to snatch it!",
							the(xname(obj)));
				}
				place_object(obj, u.ux, u.uy);
			    }
#endif /* 0 */
			    break;
			default:
			{
			    /* to floor beneath mon */
			    You("knock %s from %s grasp!",
				the(xname(obj)),
				s_suffix(mon_nam(mtmp)));
			    if(obj->otyp == CRYSKNIFE)
				obj->otyp = WORM_TOOTH;
			    place_object(obj, mtmp->mx, mtmp->my);
			}
		    }
		} else {
			pline("%s evades your attack.",Monnam(mtmp));
		}
		break;
	    case T_DAZZLE:
	    	if (Blind) {
	    		You("can't see anything!");
	    		return(0);
	    	}
	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			You("can't see yourself!");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!canseemon(mtmp)) {
			You("don't see anything there!");
			return (0);
		}
                You("stare at %s.", mon_nam(mtmp));
                if (!haseyes(mtmp->data))
                	pline("..but %s has no eyes!", mon_nam(mtmp));
                else if (!mtmp->mcansee)
                	pline("..but %s cannot see you!", mon_nam(mtmp));
                if ((rn2(6) + rn2(6) + (techlev(tech_no) - mtmp->m_lev)) > 10) {
			You("dazzle %s!", mon_nam(mtmp));
			mtmp->mcanmove = 0;
			mtmp->mfrozen = rnd(10);
		} else {
                       pline("%s breaks the stare!", Monnam(mtmp));
		}
               	techtout(tech_no) = rn1(50,25);
	    	break;
	    case T_BLITZ:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("can't do this while wielding a weapon!");
	    		return(0);
	    	} else if (uarms) {
			You("can't do this while holding a shield!");
	    		return(0);
	    	}
	    	if (!doblitz()) return (0);		
		
                techtout(tech_no) = rn1(1000,500);
	    	break;
            case T_PUMMEL:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("can't do this while wielding a weapon!");
	    		return(0);
	    	} else if (uarms) {
			You("can't do this while holding a shield!");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("flex your muscles.");
			return(0);
		}
            	if (!blitz_pummel()) return(0);
                techtout(tech_no) = rn1(1000,500);
		break;
            case T_G_SLAM:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("can't do this while wielding a weapon!");
	    		return(0);
	    	} else if (uarms) {
			You("can't do this while holding a shield!");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("flex your muscles.");
			return(0);
		}
            	if (!blitz_g_slam()) return(0);
                techtout(tech_no) = rn1(1000,500);
		break;
            case T_DASH:
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			You("stretch.");
			return(0);
		}
            	if (!blitz_dash()) return(0);
                techtout(tech_no) = rn1(50, 25);
		break;
            case T_POWER_SURGE:
            	if (!blitz_power_surge()) return(0);
		techtout(tech_no) = rn1(1000,500);
		break;            	
            case T_SPIRIT_BOMB:
	    	if (uwep || (u.twoweap && uswapwep)) {
			You("can't do this while wielding a weapon!");
	    		return(0);
	    	} else if (uarms) {
			You("can't do this while holding a shield!");
	    		return(0);
	    	}
		if (!getdir((char *)0)) return(0);
            	if (!blitz_spirit_bomb()) return(0);
		techtout(tech_no) = rn1(1000,500);
		break;            	
	    default:
	    	pline ("Error!  No such effect (%i)", tech_no);
		break;
	  }
	/*By default,  action should take a turn*/
	return(1);
}

/* Whether or not a tech is in use.
 * 0 if not in use, turns left if in use. Tech is done when techinuse == 1
 */
int
tech_inuse(tech_id)
int tech_id;
{
        int i;

        if (tech_id < 1 || tech_id > MAXTECH) {
                impossible ("invalid tech: %d", tech_id);
                return(0);
        }
        for (i = 0; i < MAXTECH && techid(i) != NO_TECH; i++) {
                if (techid(i) == tech_id) {
                        return (techt_inuse(i));
                }
        }
	return (0);
}

/* Whether or not you have a technique.
 * 
 */
boolean
have_tech(tech_id)
int tech_id;
{
        int i;

        if (tech_id < 1 || tech_id > MAXTECH) {
                impossible ("invalid tech: %d", tech_id);
                return(FALSE);
        }
        for (i = 0; i < MAXTECH && techid(i) != NO_TECH; i++) 
            return TRUE;
	return (FALSE);
}

void
tech_timeout()
{
	int i;
	
        for (i = 0; i < MAXTECH && techid(i) != NO_TECH; i++) {
	    if (techt_inuse(i)) {
	    	/* Check if technique is done */
	        if (!(--techt_inuse(i)))
	        switch (techid(i)) {
		    case T_EVISCERATE:
			You("retract your claws.");
			/* You're using bare hands now,  so new msg for next attack */
			unweapon=TRUE;
			/* Lose berserk status */
			repeat_hit = 0;
			break;
		    case T_BERSERK:
			The("red haze in your mind clears.");
			break;
		    case T_KIII:
			You("calm down.");
			break;
		    case T_FLURRY:
			You("relax.");
			break;
		    case T_E_FIST:
			You("feel the power dissipate.");
			break;
		    case T_SIGIL_TEMPEST:
			pline_The("sigil of tempest fades.");
			break;
		    case T_SIGIL_CONTROL:
			pline_The("sigil of control fades.");
			break;
		    case T_SIGIL_DISCHARGE:
			pline_The("sigil of discharge fades.");
			break;
		    case T_RAGE:
			Your("anger cools.");
			break;
		    case T_POWER_SURGE:
			pline_The("awesome power within you fades.");
			break;
		    case T_BLINK:
			You("sense the flow of time returning to normal.");
			break;
		    case T_CHI_STRIKE:
			You("feel the power in your hands dissipate.");
			break;
		    case T_CHI_HEALING:
			You("feel the healing power dissipate.");
			break;
	            default:
	            	break;
	        } else switch (techid(i)) {
	        /* During the technique */
		    case T_RAGE:
			/* Bleed but don't kill */
			if (u.uhpmax > 1) u.uhpmax--;
			if (u.uhp > 1) u.uhp--;
		    case T_POWER_SURGE:
			/* Bleed off power */
			if (u.uenmax > 1) u.uenmax--;
			if (u.uen > 0) u.uen--;
	            default:
	            	break;
	        }
	    } 

	    if (techtout(i)) techtout(i)--;
        }
}

void
docalm()
{
	int i, n = 0;

	for (i = 0; i < MAXTECH && techid(i) != NO_TECH; i++)
		if (techt_inuse(i)) {
			techt_inuse(i) = 0;
			n++;
		}
	if (n)
		You("calm down.");
	return;
}

static void
hurtmon(mtmp, tmp)
struct monst *mtmp;
int tmp;
{
	mtmp->mhp -= tmp;
	if (mtmp->mhp < 1) killed (mtmp);
#ifdef SHOW_DMG
	else showdmg(tmp);
#endif
}

static const struct 	innate_tech *
role_tech()
{
	switch (Role_switch) {
		case PM_ARCHEOLOGIST:	return (arc_tech);
		case PM_BARBARIAN:	return (bar_tech);
		case PM_CAVEMAN:	return (cav_tech);
		case PM_FLAME_MAGE:	return (fla_tech);
		case PM_HEALER:		return (hea_tech);
		case PM_ICE_MAGE:	return (ice_tech);
		case PM_KNIGHT:		return (kni_tech);
		case PM_MONK: 		return (mon_tech);
		case PM_NECROMANCER:	return (nec_tech);
		case PM_PRIEST:		return (pri_tech);
		case PM_RANGER:		return (ran_tech);
		case PM_ROGUE:		return (rog_tech);
		case PM_SAMURAI:	return (sam_tech);
#ifdef TOURIST        
		case PM_TOURIST:	return (tou_tech);
#endif        
		case PM_UNDEAD_SLAYER:	return (und_tech);
		case PM_VALKYRIE:	return (val_tech);
		case PM_WIZARD:		return (wiz_tech);
#ifdef YEOMAN
		case PM_YEOMAN:		return (yeo_tech);
#endif
		default: 		return ((struct innate_tech *) 0);
	}
}

static const struct     innate_tech *
race_tech()
{
	switch (Race_switch) {
		case PM_DOPPELGANGER:	return (dop_tech);
#ifdef DWARF
		case PM_DWARF:		return (dwa_tech);
#endif
		case PM_ELF:
		case PM_DROW:		return (elf_tech);
		case PM_GNOME:		return (gno_tech);
		case PM_HOBBIT:		return (hob_tech);
		case PM_HUMAN_WEREWOLF:	return (lyc_tech);
		case PM_VAMPIRE:	return (vam_tech);
		default: 		return ((struct innate_tech *) 0);
	}
}

void
adjtech(oldlevel,newlevel)
int oldlevel, newlevel;
{
	const struct   innate_tech  
		*tech = role_tech(), *rtech = race_tech();
	short i;

	while (tech || rtech) {
	    /* Have we finished with the tech lists? */
	    if (!tech || !tech->tech_id) {
	    	/* Try the race intrinsics */
	    	if (!rtech || !rtech->tech_id) break;
	    	tech = rtech;
	    	rtech = (struct innate_tech *) 0;
	    }
		
	    for(; tech->tech_id; tech++)
		if(oldlevel < tech->ulevel && newlevel >= tech->ulevel) {
			learntech(tech->tech_id, tech->tech_lev);
			if (tech->ulevel != 1)
				You("learn how to perform %s!", tech_names[tech->tech_id]);
		} else if (oldlevel >= tech->ulevel && newlevel < tech->ulevel
		    && tech->ulevel != 1) {
			i = tech->tech_id;
			learntech(tech->tech_id, -1);
			You("lose the ability to perform %s!", tech_names[tech->tech_id]);
		}
	}
}

int
mon_to_zombie(monnum)
int monnum;
{
	if ((&mons[monnum])->mlet == S_ZOMBIE) return monnum;  /* is already zombie */
	if ((&mons[monnum])->mlet == S_KOBOLD) return PM_KOBOLD_ZOMBIE;
	if ((&mons[monnum])->mlet == S_GNOME) return PM_GNOME_ZOMBIE;
	if (is_orc(&mons[monnum])) return PM_ORC_ZOMBIE;
	if (is_elf(&mons[monnum])) return PM_ELF_ZOMBIE;
	if (is_human(&mons[monnum])) return PM_HUMAN_ZOMBIE;
	if (monnum == PM_ETTIN) return PM_ETTIN_ZOMBIE;
	if (is_giant(&mons[monnum])) return PM_GIANT_ZOMBIE;
	/* Is it humanoid? */
	if (!humanoid(&mons[monnum])) return (-1);
	/* Otherwise,  return a ghoul or ghast */
	if (!rn2(4)) return PM_GHAST;
	else return PM_GHOUL;
}


/*WAC tinker code*/
STATIC_PTR int
tinker()
{
	int chance;
	struct obj *otmp = uwep;


	if (delay) {    /* not if (delay++), so at end delay == 0 */
		delay++;
#if 0
		use_skill(P_TINKER, 1); /* Tinker skill */
#endif
		/*WAC a bit of practice so even if you're interrupted
		you won't be wasting your time ;B*/
		return(1); /* still busy */
	}

	if (!uwep)
		return (0);

	You("finish your tinkering.");
	chance = 5;
/*	chance += PSKILL(P_TINKER); */
	if (rnl(10) < chance) {		
		upgrade_obj(otmp);
	} else {
		/* object downgrade  - But for now,  nothing :) */
	}

	setuwep(otmp);
	You("now hold %s!", doname(otmp));
	return(0);
}

/*WAC  draw energy from surrounding objects */
STATIC_PTR int
draw_energy()
{
	int powbonus = 1;
	if (delay) {    /* not if (delay++), so at end delay == 0 */
		delay++;
		confdir();
		if(isok(u.ux + u.dx, u.uy + u.dy)) {
			switch((&levl[u.ux + u.dx][u.uy + u.dy])->typ) {
			    case ALTAR: /* Divine power */
			    	powbonus =  (u.uenmax > 28 ? u.uenmax / 4
			    			: 7);
				break;
			    case THRONE: /* Regal == pseudo divine */
			    	powbonus =  (u.uenmax > 36 ? u.uenmax / 6
			    			: 6);			    		 	
				break;
			    case CLOUD: /* Air */
			    case TREE: /* Earth */
			    case LAVAPOOL: /* Fire */
			    case ICE: /* Water - most ordered form */
			    	powbonus = 5;
				break;
			    case AIR:
			    case MOAT: /* Doesn't freeze */
			    case WATER:
			    	powbonus = 4;
				break;
			    case POOL: /* Can dry up */
			    	powbonus = 3;
				break;
			    case FOUNTAIN:
			    	powbonus = 2;
				break;
			    case SINK:  /* Cleansing water */
			    	if (!rn2(3)) powbonus = 2;
				break;
			    case TOILET: /* Water Power...but also waste! */
			    	if (rn2(100) < 50)
			    		powbonus = 2;
			    	else powbonus = -2;
				break;
			    case GRAVE:
			    	powbonus = -4;
				break;
			    default:
				break;
			}
		}
		u.uen += powbonus;
		if (u.uen > u.uenmax) {
			delay = 0;
			u.uen = u.uenmax;
		}
		if (u.uen < 1) u.uen = 0;
		flags.botl = 1;
		return(1); /* still busy */
	}
	You("finish drawing energy from your surroundings.");
	return(0);
}

static const char 
	*Enter_Blitz = "Enter Blitz Command[. to end]: ";

/* Keep commands that reference the same blitz together 
 * Keep the BLITZ_START before the BLITZ_CHAIN before the BLITZ_END
 */
static const struct blitz_tab blitzes[] = { 	
	{"LLDDR", 5, blitz_chi_strike, T_CHI_STRIKE, BLITZ_START},
	{"LLDDRDR", 7, blitz_chi_strike, T_CHI_STRIKE, BLITZ_START},
	{"RR",  2, blitz_dash, T_DASH, BLITZ_START},
	{"LL",  2, blitz_dash, T_DASH, BLITZ_START},
	{"UURRDDL", 7, blitz_e_fist, T_E_FIST, BLITZ_START},
	{"URURRDDLDL", 10, blitz_e_fist, T_E_FIST, BLITZ_START},
	{"DDRRDDRR", 8, blitz_power_surge, T_POWER_SURGE, BLITZ_START},
	{"DRDRDRDR", 8, blitz_power_surge, T_POWER_SURGE, BLITZ_START},
	{"LRL", 3, blitz_pummel, T_PUMMEL, BLITZ_CHAIN},
	{"RLR", 3, blitz_pummel, T_PUMMEL, BLITZ_CHAIN},
	{"DDDD", 4, blitz_g_slam, T_G_SLAM, BLITZ_END},
	{"DUDUUDDD", 8, blitz_spirit_bomb, T_SPIRIT_BOMB, BLITZ_END},
	{"", 0, (void *)0, 0, BLITZ_END} /* Array terminator */
};

#define MAX_BLITZ 50
#define MIN_CHAIN 2
#define MAX_CHAIN 5

/* parse blitz input */
static int
doblitz()
{
	int i, j, dx, dy, done = 0, tech_no;
	char buf[BUFSZ];
	char *bp;
	int blitz_chain[MAX_CHAIN], blitz_num;
        
	tech_no = (get_tech_no(T_BLITZ));

	if (tech_no == -1) {
		return(0);
	}
	
	if (u.uen < 10) {
		You("are too weak to attempt this!");
            	return(0);
	}

	bp = buf;
	
	if (!getdir((char *)0)) return(0);
	if (!u.dx && !u.dy) {
		return(0);
	}
	
	dx = u.dx;
	dy = u.dy;

	doblitzlist();

    	for (i= 0; i < MAX_BLITZ; i++) {
		if (!getdir(Enter_Blitz)) return(0); /* Get directional input */
    		if (!u.dx && !u.dy && !u.dz) break;
    		if (u.dx == -1) {
    			*(bp) = 'L';
    			bp++;
    		} else if (u.dx == 1) {
    			*(bp) = 'R';
    			bp++;
    		}
    		if (u.dy == -1) {
    			*(bp) = 'U';
    			bp++;
    		} else if (u.dy == 1) {
    			*(bp) = 'D';
    			bp++;
    		}
    		if (u.dz == -1) {
    			*(bp) = '>';
    			bp++;
    		} else if (u.dz == 1) {
    			*(bp) = '<';
    			bp++;
    		}
    	}
	*(bp) = '.';
	bp++;
	*(bp) = '\0';
	bp = buf;

	/* Point of no return - You've entered and terminated a blitz, so... */
    	u.uen -= 10;

    	/* parse input */
    	/* You can't put two of the same blitz in a row */
    	blitz_num = 0;
    	while(strncmp(bp, ".", 1)) {
	    done = 0;
	    for (j = 0; blitzes[j].blitz_len; j++) {
	    	if (blitz_num >= MAX_CHAIN || 
	    	    blitz_num >= (MIN_CHAIN + (techlev(tech_no) / 10)))
	    		break; /* Trying to chain too many blitz commands */
		else if (!strncmp(bp, blitzes[j].blitz_cmd, blitzes[j].blitz_len)) {
	    		/* Trying to chain in a command you don't know yet */
			if (!tech_known(blitzes[j].blitz_tech))
				break;
	    		if (blitz_num) {
				/* Check if trying to chain two of the exact same 
				 * commands in a row 
				 */
	    			if (j == blitz_chain[(blitz_num - 1)]) 
	    				break;
	    			/* Trying to chain after chain finishing command */
	    			if (blitzes[blitz_chain[(blitz_num - 1)]].blitz_type 
	    							== BLITZ_END)
	    				break;
	    			/* Trying to put a chain starter after starting
	    			 * a chain
	    			 * Note that it's OK to put two chain starters in a 
	    			 * row
	    			 */
	    			if ((blitzes[j].blitz_type == BLITZ_START) &&
	    			    (blitzes[blitz_chain[(blitz_num - 1)]].blitz_type 
	    							!= BLITZ_START))
	    				break;
	    		}
			bp += blitzes[j].blitz_len;
			blitz_chain[blitz_num] = j;
			blitz_num++;
			done = 1;
			break;
		}
	    }
	    if (!done) {
		You("stumble!");
		return(1);
	    }
    	}
	for (i = 0; i < blitz_num; i++) {
	    u.dx = dx;
	    u.dy = dy;
	    if (!( (*blitzes[blitz_chain[i]].blitz_funct)() )) break;
	}
	
    	/* done */
	return(1);
}

static void
doblitzlist()
{
	winid tmpwin;
	int i, n;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

        Sprintf(buf, "%16s %10s %-17s", "[LU = Left Up]", "[U = Up]", "[RU = Right Up]");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
        Sprintf(buf, "%16s %10s %-17s", "[L = Left]", "", "[R = Right]");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
        Sprintf(buf, "%16s %10s %-17s", "[LD = Left Down]", "[D = Down]", "[RD = Right Down]");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);        

        Sprintf(buf, "%-30s %10s   %s", "Name", "Type", "Command");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

        for (i = 0; blitzes[i].blitz_len; i++) {
	    if (tech_known(blitzes[i].blitz_tech)) {
                Sprintf(buf, "%-30s %10s   %s",
                    (i && blitzes[i].blitz_tech == blitzes[(i-1)].blitz_tech ?
                    	"" : tech_names[blitzes[i].blitz_tech]), 
                    (blitzes[i].blitz_type == BLITZ_START ? 
                    	"starter" :
                    	(blitzes[i].blitz_type == BLITZ_CHAIN ? 
	                    	"chain" : 
	                    	(blitzes[i].blitz_type == BLITZ_END ? 
                    			"finisher" : "unknown"))),
                    blitzes[i].blitz_cmd);

		add_menu(tmpwin, NO_GLYPH, &any,
                         0, 0, ATR_NONE, buf, MENU_UNSELECTED);
	    }
	}
        end_menu(tmpwin, "Currently known blitz manoeuvers");

	n = select_menu(tmpwin, PICK_NONE, &selected);
	destroy_nhwindow(tmpwin);
	return;
}

static int
blitz_chi_strike()
{
	int tech_no;
	
	tech_no = (get_tech_no(T_CHI_STRIKE));

	if (tech_no == -1) {
		return(0);
	}

	if (u.uen < 1) {
		You("are too weak to attempt this!");
            	return(0);
	}
	You("feel energy surge through your hands!");
	techt_inuse(tech_no) = techlev(tech_no) + 4;
	return(1);
}

static int
blitz_e_fist()
{
	int tech_no;
	const char *str;
	
	tech_no = (get_tech_no(T_E_FIST));

	if (tech_no == -1) {
		return(0);
	}
	
	str = makeplural(body_part(HAND));
	You("focus the powers of the elements into your %s.", str);
	techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/3 + 1)) + d(1,4) + 2;
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_pummel()
{
	int i = 0, tech_no;
	struct monst *mtmp;
	tech_no = (get_tech_no(T_PUMMEL));

	if (tech_no == -1) {
		return(0);
	}

	You("let loose a barrage of blows!");

	mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	if (!mtmp) {
		You("strike nothing.");
		return (0);
	}
	if (!attack(mtmp)) return (0);
	
	/* Perform the extra attacks
	 */
	for (i = 0; (i < 4); i++) {
	    if (rn2(70) > (techlev(tech_no) + 30)) break;
	    mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	    if (!mtmp) return (1);
	    if (!attack(mtmp)) return (1);
	} 
	
	return(1);
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_g_slam()
{
	int i = 0, tech_no, tmp;
	struct monst *mtmp;
	struct trap *chasm;

	tech_no = (get_tech_no(T_G_SLAM));

	if (tech_no == -1) {
		return(0);
	}

	mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	if (!mtmp) {
		You("strike nothing.");
		return (0);
	}
	if (!attack(mtmp)) return (0);
	
	/* Slam the monster into the ground */
	mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
	if (!mtmp || u.uswallow) return(1);
	
	You("hurl %s downwards...", mon_nam(mtmp));
	if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) return(1);
	
	tmp = (5 + rnd(6) + (techlev(tech_no) / 5));
	
	chasm = maketrap(u.ux + u.dx, u.uy + u.dy,PIT);
	if (chasm) {
		chasm->tseen = 1;
		levl[(u.ux + u.dx)][(u.uy + u.dy)].doormask = 0;
		pline("%s slams into the ground, creating a crater!", Monnam(mtmp));
		tmp *= 2;
	}

	mselftouch(mtmp, "Falling, ", TRUE);
	if (!DEADMONSTER(mtmp))
	    if ((mtmp->mhp -= tmp) <= 0) {
		if(!cansee(u.ux + u.dx, u.uy + u.dy))
		    pline("It is destroyed!");
		else {
		    You("destroy %s!", 	
		    	mtmp->mtame
			    ? x_monnam(mtmp, ARTICLE_THE, "poor",
				mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE)
			    : mon_nam(mtmp));

		}
		xkilled(mtmp,0);
	}

	return(1);
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_dash()
{
	int i = 0, tech_no;
	struct monst *mtmp;
	tech_no = (get_tech_no(T_DASH));

	if (tech_no == -1) {
		return(0);
	}
	
	You("dash forwards!");
	dash(u.dx, u.dy, 2, FALSE);
}

static int
blitz_power_surge()
{
	int tech_no, num;
	
	tech_no = (get_tech_no(T_POWER_SURGE));

	if (tech_no == -1) {
		return(0);
	}

	if (Upolyd) {
		You("cannot tap into your full potential in this form.");
		return(0);
	}
    	You("tap into the full extent of your power!");
	num = 50 + (2 * techlev(tech_no));
    	techt_inuse(tech_no) = num + 1;
	u.uenmax += num;
	u.uen = u.uenmax;
}

/* Assumes u.dx, u.dy already set up */
static int
blitz_spirit_bomb()
{
	int tech_no, num;
	int sx = u.ux, sy = u.uy, i;
	
	tech_no = (get_tech_no(T_SPIRIT_BOMB));

	if (tech_no == -1) {
		return(0);
	}

	You("gather your energy...");
	
	if (u.uen < 10) {
		pline("But it fizzles out.");
		u.uen = 0;
	}

	num = 10 + (techlev(tech_no) / 5);
	num = (u.uen < num ? u.uen : num);
	
	u.uen -= num;
	
	for( i = 0; i < 2; i++) {		
	    if (!isok(sx,sy) || !cansee(sx,sy) || 
	    		IS_STWALL(levl[sx][sy].typ) || u.uswallow)
	    	break;

	    /* Display the center of the explosion */
	    tmp_at(DISP_FLASH, cmap_to_glyph(S_mexplode5));
	    tmp_at(sx, sy);
	    delay_output();
	    tmp_at(DISP_END, 0);

	    sx += u.dx;
	    sy += u.dy;
	}
	/* Magical Explosion */
	explode(sx, sy, 10, (d(3,6) + num), WAND_CLASS);
}

/*
 * The player moves through the air for a few squares.  This is pretty much
 * a copy of an older version of hurtle from dothrow.c,  without the nomul()
 */
STATIC_OVL void
dash(dx, dy, range, verbos)
	int dx, dy, range;
	boolean verbos;
{
    register struct monst *mon;
    struct obj *obj;
    int nx, ny;

    /* The chain is stretched vertically, so you shouldn't be able to move
     * very far diagonally.  The premise that you should be able to move one
     * spot leads to calculations that allow you to only move one spot away
     * from the ball, if you are levitating over the ball, or one spot
     * towards the ball, if you are at the end of the chain.  Rather than
     * bother with all of that, assume that there is no slack in the chain
     * for diagonal movement, give the player a message and return.
     */
    if(Punished && !carried(uball)) {
		if (verbos) You_feel("a tug from the iron ball.");
		return;
    } else if (u.utrap) {
		if (verbos) You("are anchored by the %s.",
		    u.utraptype == TT_WEB ? "web" : u.utraptype == TT_LAVA ? "lava" :
			u.utraptype == TT_INFLOOR ? surface(u.ux,u.uy) : "trap");
		return;
    }

    if(!range || (!dx && !dy) || u.ustuck) return; /* paranoia */

    if (verbos) You("%s in the opposite direction.", range > 1 ?
    		"hurtle" : "float");
    while(range--) {
	nx = u.ux + dx;
	ny = u.uy + dy;

	if(!isok(nx,ny)) break;
	if(IS_ROCK(levl[nx][ny].typ) || closed_door(nx,ny) ||
	   (IS_DOOR(levl[nx][ny].typ) && (levl[nx][ny].doormask & D_ISOPEN))) {
	    pline("Ouch!");
	    losehp(rnd(2+range), IS_ROCK(levl[nx][ny].typ) ?
		   "bumping into a wall" : "bumping into a door", KILLED_BY);
	    break;
	}

	if ((obj = sobj_at(BOULDER,nx,ny)) != 0) {
	    You("bump into a %s.  Ouch!", xname(obj));
	    losehp(rnd(2+range), "bumping into a boulder", KILLED_BY);
	    break;
	}

	u.ux = nx;
	u.uy = ny;
	newsym(u.ux - dx, u.uy - dy);
	if ((mon = m_at(u.ux, u.uy)) != 0) {
	    You("bump into %s.", a_monnam(mon));
	    wakeup(mon);
	    if(Is_airlevel(&u.uz))
		mnexto(mon);
	    else {
		/* sorry, not ricochets */
		u.ux -= dx;
		u.uy -= dy;
	    }
	    range = 0;
	}

	vision_recalc(1);               /* update for new position */

	if(range) {
	    flush_screen(1);
	    delay_output();
	}
    }
}

