#include "cg_local.h"

#define MAX_HUDFILELENGTH 20000
#define MAX_PARAMETER 4
#define MAX_TOKENNUM 2048

typedef struct {
	char *name;
	int parameterNum;
	int parameterTypes[MAX_PARAMETER];
} hudElementProperties_t;

hudElementProperties_t hudProperties[10] = {
	{ (char*)"rect", 4, { TOT_NUMBER, TOT_NUMBER, TOT_NUMBER, TOT_NUMBER } },
	{ (char*)"bgcolor", 4, { TOT_NUMBER, TOT_NUMBER, TOT_NUMBER, TOT_NUMBER } },
	{ (char*)"color", 4, { TOT_NUMBER, TOT_NUMBER, TOT_NUMBER, TOT_NUMBER } },
	{ (char*)"fill", 0, { TOT_NIL } },
	{ (char*)"fontsize", 2, { TOT_NUMBER, TOT_NUMBER } },
	{ (char*)"image", 1, { TOT_WORD } },
	{ (char*)"text", 1, { TOT_WORD } },
	{ (char*)"textalign", 1, { TOT_WORD } },
	{ (char*)"time", 1, { TOT_NUMBER } },
	{ (char*)"textstyle", 1, { TOT_NUMBER } }
};

static const char *HudNames[] =
{
    "!DEFAULT",
    "AmmoMessage",
    "AttackerIcon",
    "AttackerName",
    "Chat1",
    "Chat2",
    "Chat3",
    "Chat4",
    "Chat5",
    "Chat6",
    "Chat7",
    "Chat8",
    "FlagStatus_OWN",
    "FlagStatus_NME",
    "FollowMessage",
    "FPS",
    "FragMessage",
    "GameTime",
    "RoundTime",
    "GameType",
    "ItemPickupName",
    "ItemPickupTime",
    "ItemPickupIcon",
    "NetGraph",
    "NetGraphPing",
    "PlayerSpeed",
    "PlayerAccel",
    "PowerUp1_Time",
    "PowerUp2_Time",
    "PowerUp3_Time",
    "PowerUp4_Time",
    "PowerUp1_Icon",
    "PowerUp2_Icon",
    "PowerUp3_Icon",
    "PowerUp4_Icon",
    "RankMessage",
    "Score_Limit",
    "Score_NME",
    "Score_OWN",
    "SpecMessage",
    "StatusBar_ArmorBar",
    "StatusBar_ArmorCount",
    "StatusBar_ArmorIcon",
    "StatusBar_AmmoBar",
    "StatusBar_AmmoCount",
    "StatusBar_AmmoIcon",
    "StatusBar_HealthBar",
    "StatusBar_HealthCount",
    "StatusBar_HealthIcon",
    "TargetName",
    "TargetStatus",
    "TeamCount_NME",
    "TeamCount_OWN",
    "TeamIcon_NME",
    "TeamIcon_OWN",
    "Team1",
    "Team2",
    "Team3",
    "Team4",
    "Team5",
    "Team6",
    "Team7",
    "Team8",
    "VoteMessage",
    "WarmupInfo",
    "WeaponList",
    "ReadyStatus",
    "DeathNotice1",
    "DeathNotice2",
    "DeathNotice3",
    "DeathNotice4",
    "DeathNotice5",
    "Countdown",
    "PreDecorate1",
    "PreDecorate2",
    "PreDecorate3",
    "PreDecorate4",
    "PreDecorate5",
    "PreDecorate6",
    "PreDecorate7",
    "PreDecorate8",
    "PostDecorate1",
    "PostDecorate2",
    "PostDecorate3",
    "PostDecorate4",
    "PostDecorate5",
    "PostDecorate6",
    "PostDecorate7",
    "PostDecorate8",
    
    "HUD_MAX"
}; 

int CG_setTokens( char* in, char* out, int start ){
	int i = 0;
	while ( in[ start + i ] != ' ' ){
		if( in[ start + i ] == '\0' ){
			out[i] = in[start+1];
			return MAX_HUDFILELENGTH;
		}
		out[i] = in[start+i];
		i++;
	}
	out[i] = '\0';
	return start+i+1;
}

int CG_setTokenType( char* value ){
	int count = 0;
	qboolean lpar= qfalse,rpar= qfalse,number= qfalse, character = qfalse;
	
	while( value[count] != '\0' ){
		if( value[count] == '{' )
			lpar = qtrue;
		else if( value[count] == '}' )
			rpar = qtrue;
		else if( value[count] >= '0' && value[count] <= '9' )
			number = qtrue;
		else if( ( value[count] >= 'a' && value[count] <= 'z' ) || ( value[count] >= 'A' && value[count] <= 'Z' ) )
			character = qtrue;
		count++;
	}
	
	if( lpar && !( rpar || number || character ) )
		return TOT_LPAREN;
	else if( rpar && !( lpar || number || character ) )
		return TOT_RPAREN;
	else if( number && !( lpar || rpar || character ) )
		return TOT_NUMBER;
	else if( character && !( lpar || rpar ) )
		return TOT_WORD;
	else
		return TOT_NIL;
}

qboolean SkippedChar( char in ){
	return( in == '\n' || in == '\r' || in == ';' || in == '\t' || in == ' ' );
}

int CG_HudElement( token_t in ){
	int i;
	int name;
	
	for( i = 0; i < HUD_MAX; i++ ){
		name = strcmp( in.value, HudNames[i] );
		if( name == 0 )
			return i;
	}
	return -1;
}

int CG_FindNextToken( char *find, token_t *in, int start ){
	int i;
	int cmp;
	
	for( i = start; i < MAX_TOKENNUM; i++ ){
		cmp= strcmp( in[i].value, find );
		if( cmp == 0 )
			return i;
	}
	return -1;
}

qboolean CG_AbeforeB( char *A, char *B, token_t *in, int start ){
	int a = CG_FindNextToken( A, in, start );
	int b = CG_FindNextToken( B, in, start );
	
	if( b == -1 && a != -1 )
		return qtrue;
	if( a == -1 && b != -1 )
		return qfalse;
	if( a < b )
		return qtrue;
	else
		return qfalse;
}

static void CG_SetHudRect( int hudnumber, char* arg1, char* arg2, char* arg3, char* arg4 ){
	cgs.hud[hudnumber].xpos = atoi(arg1);
	cgs.hud[hudnumber].ypos = atoi(arg2);
	cgs.hud[hudnumber].width = atoi(arg3);
	cgs.hud[hudnumber].height = atoi(arg4);
}

static void CG_SetHudBGColor( int hudnumber, char* arg1, char* arg2, char* arg3, char* arg4 ){
	cgs.hud[hudnumber].bgcolor[0] = atof(arg1);
	cgs.hud[hudnumber].bgcolor[1] = atof(arg2);
	cgs.hud[hudnumber].bgcolor[2] = atof(arg3);
	cgs.hud[hudnumber].bgcolor[3] = atof(arg4);
}

static void CG_SetHudColor( int hudnumber, char* arg1, char* arg2, char* arg3, char* arg4 ){
	cgs.hud[hudnumber].color[0] = atof(arg1);
	cgs.hud[hudnumber].color[1] = atof(arg2);
	cgs.hud[hudnumber].color[2] = atof(arg3);
	cgs.hud[hudnumber].color[3] = atof(arg4);
}

static void CG_SetHudFontsize( int hudnumber, char* arg1, char* arg2 ){
	cgs.hud[hudnumber].fontWidth = atoi(arg1);
	cgs.hud[hudnumber].fontHeight = atoi(arg2);
}

static void CG_SetHudImage( int hudnumber, char* arg1 ){
	cgs.hud[hudnumber].image = arg1;
	//CG_Printf("%s\n", arg1);
	cgs.hud[hudnumber].imageHandle = trap_R_RegisterShader( arg1 );
}

static void CG_SetHudText( int hudnumber, char* arg1 ){
	cgs.hud[hudnumber].text = arg1;
}

static void CG_SetHudFill( int hudnumber ){
	cgs.hud[hudnumber].fill = qtrue;
}

static void CG_SetHudTextalign( int hudnumber, char* arg1 ){
	if( strcmp( arg1, "L") == 0 )
		cgs.hud[hudnumber].textAlign = 0;
	else if( strcmp( arg1, "R" ) == 0 )
		cgs.hud[hudnumber].textAlign = 2;
	else
		cgs.hud[hudnumber].textAlign = 1;
}

static void CG_SetHudTime( int hudnumber, char* arg1 ){
	cgs.hud[hudnumber].time = atoi(arg1);
}

static void CG_SetTextStyle( int hudnumber, char* arg1 ){
	cgs.hud[hudnumber].textstyle = atoi(arg1);
}

static void CG_setHudElement( int hudnumber, token_t *in, int min, int max ){
	int i,j,k;
	qboolean rect=qfalse, bgcolor=qfalse, color=qfalse, fill=qfalse, fontsize=qfalse, image=qfalse, text=qfalse, textalign=qfalse, time=qfalse, textstyle = qfalse;
	
	//Syntax check and parsing
	for( i = min; i <= max; i++ ){
		for( j = 0; j < 10; j++ ){
			if( strcmp( in[i].value, hudProperties[j].name ) == 0 ){
				for( k = 0; k < hudProperties[j].parameterNum; k++ ){
					if( hudProperties[j].parameterTypes[k] != in[i+1+k].type ){
						CG_Printf("Syntaxerror: %s in %s\n", in[i].value, HudNames[hudnumber] );
						return;
					}
				}
				if( strcmp( in[i].value, "rect" ) == 0 ){
					CG_SetHudRect( hudnumber, in[i+1].value, in[i+2].value, in[i+3].value, in[i+4].value );
					rect = qtrue;
				}
				else if( strcmp( in[i].value, "bgcolor" ) == 0 ){
					CG_SetHudBGColor( hudnumber, in[i+1].value, in[i+2].value, in[i+3].value, in[i+4].value );
					bgcolor = qtrue;
				}
				else if( strcmp( in[i].value, "color" ) == 0 ){
					CG_SetHudColor( hudnumber, in[i+1].value, in[i+2].value, in[i+3].value, in[i+4].value );
					color = qtrue;
				}
				else if( strcmp( in[i].value, "fill" ) == 0 ){
					CG_SetHudFill( hudnumber );
					fill = qtrue;
				}
				else if( strcmp( in[i].value, "fontsize" ) == 0 ){
					CG_SetHudFontsize( hudnumber, in[i+1].value, in[i+2].value );
					fontsize = qtrue;
				}
				else if( strcmp( in[i].value, "image" ) == 0 ){
					CG_SetHudImage( hudnumber, in[i+1].value );
					image = qtrue;
				}
				else if( strcmp( in[i].value, "text" ) == 0 ){
					CG_SetHudText( hudnumber, in[i+1].value );
					text = qtrue;
				}
				else if( strcmp( in[i].value, "textalign" ) == 0 ){
					CG_SetHudTextalign( hudnumber, in[i+1].value );
					textalign = qtrue;
				}
				else if( strcmp( in[i].value, "time" ) == 0 ){
					CG_SetHudTime( hudnumber, in[i+1].value );
					time = qtrue;
				}
				else if( strcmp( in[i].value, "textstyle" ) == 0 ){
					CG_SetTextStyle( hudnumber, in[i+1].value );
					textstyle = qtrue;
				}
			}
		}
	}
	
	if( hudnumber != HUD_DEFAULT ){
		if( !rect ){
			cgs.hud[hudnumber].xpos = cgs.hud[HUD_DEFAULT].xpos;
			cgs.hud[hudnumber].ypos = cgs.hud[HUD_DEFAULT].ypos;
			cgs.hud[hudnumber].width = cgs.hud[HUD_DEFAULT].width;
			cgs.hud[hudnumber].height = cgs.hud[HUD_DEFAULT].height;
		}
		if( !bgcolor ){
			cgs.hud[hudnumber].bgcolor[0] = cgs.hud[HUD_DEFAULT].bgcolor[0];
			cgs.hud[hudnumber].bgcolor[1] = cgs.hud[HUD_DEFAULT].bgcolor[1];
			cgs.hud[hudnumber].bgcolor[2] = cgs.hud[HUD_DEFAULT].bgcolor[2];
			cgs.hud[hudnumber].bgcolor[3] = cgs.hud[HUD_DEFAULT].bgcolor[3];
		}
		if( !color ){
			cgs.hud[hudnumber].color[0] = cgs.hud[HUD_DEFAULT].color[0];
			cgs.hud[hudnumber].color[1] = cgs.hud[HUD_DEFAULT].color[1];
			cgs.hud[hudnumber].color[2] = cgs.hud[HUD_DEFAULT].color[2];
			cgs.hud[hudnumber].color[3] = cgs.hud[HUD_DEFAULT].color[3];
		}
		if( !fill ){
			cgs.hud[hudnumber].fill = cgs.hud[HUD_DEFAULT].fill;
		}
		if( !fontsize ){
			cgs.hud[hudnumber].fontWidth = cgs.hud[HUD_DEFAULT].fontWidth;
			cgs.hud[hudnumber].fontHeight = cgs.hud[HUD_DEFAULT].fontHeight;
		}
		if( !image ){
			cgs.hud[hudnumber].image = cgs.hud[HUD_DEFAULT].image;
		}
		if( !text ){
			cgs.hud[hudnumber].text = cgs.hud[HUD_DEFAULT].text;
		}
		if( !textalign ){
			cgs.hud[hudnumber].textAlign = cgs.hud[HUD_DEFAULT].textAlign;
		}
		if( !time ){
			cgs.hud[hudnumber].time = cgs.hud[HUD_DEFAULT].time;
		}
		if( !textstyle ){
			cgs.hud[hudnumber].textstyle = cgs.hud[HUD_DEFAULT].textstyle;
		}
	}
	
	cgs.hud[hudnumber].inuse = qtrue;
}

void CG_WriteHudFile(){
	int i;
	
	for( i = HUD_DEFAULT+1; i < HUD_MAX; i++ ){
		
		if( !cgs.hud[i].inuse )
			continue;
		
		CG_Printf("%s {\n", HudNames[i]);
		CG_Printf("\trect %i %i %i %i\n", cgs.hud[i].xpos, cgs.hud[i].ypos, cgs.hud[i].width, cgs.hud[i].height );
		CG_Printf("\tcolor %f %3f %3f %3f\n", cgs.hud[i].color[0], cgs.hud[i].color[1], cgs.hud[i].color[2], cgs.hud[i].color[3] );
		CG_Printf("\tbgcolor %3f %3f %3f %3f\n", cgs.hud[i].bgcolor[0], cgs.hud[i].bgcolor[1], cgs.hud[i].bgcolor[2], cgs.hud[i].bgcolor[3] );
		if( cgs.hud[i].fill )
			CG_Printf("\tfill\n");
		CG_Printf("\tfontsize %i %i\n", cgs.hud[i].fontWidth, cgs.hud[i].fontHeight );
		CG_Printf("\timage %s\n", cgs.hud[i].image );
		CG_Printf("\ttext %s\n", cgs.hud[i].text );
		
		if( cgs.hud[i].textAlign == 0 )
			CG_Printf("\ttextalign L\n");
		else if( cgs.hud[i].textAlign == 2 )
			CG_Printf("\ttextalign R\n");
		else 
			CG_Printf("\ttextalign C\n");
		
		CG_Printf("\ttime %i\n", cgs.hud[i].time );
		CG_Printf("}\n\n");
	}
}

token_t tokens[MAX_TOKENNUM];

void CG_LoadHudFile( const char* hudFile ){
	char buffer[MAX_HUDFILELENGTH];
	qboolean lastSpace = qtrue;
	qboolean pgbreak = qfalse;
	int i = 0;
	int charCount = 0;
	int tokenNum = 0;
	int maxTokennum;
	int lpar, rpar;
	int len;
	fileHandle_t	f;
	
	// Default hud init
	cgs.hud[HUD_DEFAULT].text = (char*)" ";
	cgs.hud[HUD_DEFAULT].image = (char*)" ";
	cgs.hud[HUD_DEFAULT].textAlign = 1;
	
	len = trap_FS_FOpenFile( hudFile, &f, FS_READ );
	
	if ( !f ) {
		CG_Printf( "%s",va( S_COLOR_YELLOW "hud file not found: %s, using default\n", hudFile ) );
		len = trap_FS_FOpenFile( "hud/default.cfg", &f, FS_READ );
		if (!f) {
			trap_Error( va( S_COLOR_RED "default menu file not found: hud/default.cfg, unable to continue!\n") );
		}
	}

	if ( len >= MAX_HUDFILELENGTH ) {
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", hudFile, len, MAX_HUDFILELENGTH ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	
	COM_Compress(buffer);
	
	for ( i = 0; i < MAX_HUDFILELENGTH; i++ ){
		
		//Filter comments( start at # and end at break )
		if( buffer[i] == '#' ){
			while( i < MAX_HUDFILELENGTH && !pgbreak ){
				if( buffer[i] == '\n' || buffer[i] == '\r' )
					pgbreak = qtrue;
				i++;
			}
			pgbreak = qfalse;
			lastSpace = qtrue;
			//continue;
		}
		
		if( SkippedChar( buffer[i] ) ){
			if( !lastSpace ){
				buffer[charCount] = ' ';
				charCount++;
				lastSpace = qtrue;
			}
			continue;
		}
		
		lastSpace = qfalse;
		buffer[charCount] = buffer[i];
		charCount++;
	}
	
	i = 0;
	while( i < MAX_HUDFILELENGTH && tokenNum < MAX_TOKENNUM){
		i = CG_setTokens( buffer, tokens[tokenNum].value, i);
		tokens[tokenNum].type = CG_setTokenType( tokens[tokenNum].value );
		tokenNum++;
	}
	maxTokennum = tokenNum;
	
	CG_Printf("Superhud parser found %i tokens\n", maxTokennum );
	
	for( tokenNum = 0; tokenNum < maxTokennum; tokenNum++ ){
		i = CG_HudElement(tokens[tokenNum]);
		//CG_Printf("%i\n", i);
		if( i != -1 ) {
			if( strcmp( tokens[tokenNum+1].value, "{" ) == 0 ){
				//CG_Printf("lpar found\n");
				lpar = tokenNum+1;
				if( CG_AbeforeB((char*)"{",(char*)"}", tokens, tokenNum+2)){
					CG_Printf("error: \"}\" expected at %s\n", tokens[tokenNum].value);
					break;
				}
				//CG_Printf("debug abeforeb\n");
				rpar = CG_FindNextToken((char*)"}", tokens, tokenNum+2 );
				//CG_Printf("debug findnexttoken\n");
				if( rpar != 1 ){
					CG_setHudElement(i, tokens, lpar+1, rpar-1);
					tokenNum = rpar;
				}	
			}
		}	
	}
}

void CG_ClearHud( void ){
	int i;
	for( i = 0; i < HUD_MAX; i++ ){
		cgs.hud[i].bgcolor[0] = 0;
		cgs.hud[i].bgcolor[1] = 0;
		cgs.hud[i].bgcolor[2] = 0;
		cgs.hud[i].bgcolor[3] = 0;
		
		cgs.hud[i].color[0] = 0;
		cgs.hud[i].color[1] = 0;
		cgs.hud[i].color[2] = 0;
		cgs.hud[i].color[3] = 0;
		
		cgs.hud[i].fill = qfalse;
		cgs.hud[i].fontHeight = 8;
		cgs.hud[i].fontWidth = 8;
		cgs.hud[i].height = 0;
		cgs.hud[i].width = 0;
		cgs.hud[i].image = (char*)"";
		cgs.hud[i].inuse = qfalse;
		cgs.hud[i].text = (char*)"";
		cgs.hud[i].textAlign = 1;
		cgs.hud[i].textstyle = 0;
		cgs.hud[i].time = 0;
		cgs.hud[i].xpos = 0;
		cgs.hud[i].ypos = 0;
	}
}
