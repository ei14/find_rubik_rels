// Written by Thomas Kaldahl.
// No generative AI used (besides basic search queries).

// This program (as currently configured) will create 32 files.
// Combining and removing all "-" gives the full list of group relations of length 12,
// 	up to orientation.

// Clockwise rotations are denoted URFBLD as is standard; counterclockwise are urfbld.

// Each file is named:
// 	<FILESTEM><PREFIX>
// where FILESTEM is fixed and PREFIX is just the beginning bit of all relations contained.



////////////////////////////////////////////////////////////////////////////////////////////////////
// LOGIC SUMMARY
////////////////////////////////////////////////////////////////////////////////////////////////////

// The program iterates through all[*] possible algorithms of length 12.
// For each, it runs a simulation of how the 48 Rubik's cube stickers are permuted by the algorithm.
// If all stickers are at the right spots, it records the algorithm to a file.

// The program is multithreaded.
// Each thread is given a list of "prefixes."
// For each prefix, the thread will only iterate over algorithms that begin with the prefix.

// [*]: For efficiency, ~99% of all *possible* algorithms are skipped.
//
// 	Some move sequences have the same end effect, so we need only check one. E.g.:
// 		* "Uu" -> ""
// 		* "UUU" -> "u"
// 		* "uu" -> "UU"
// 		* "DU" -> "UD"
//
// 	Some algorithms can be obtained as cube-reorientations / mirrors of other algorithms.
// 	We pick a "canonical" orientation:
// 		* First move must be U.
// 		* After all the beginning UuDd moves, the next move must be R or r.
//
// See function `firstUncanon` for implementation details.



////////////////////////////////////////////////////////////////////////////////////////////////////
// CONFIG SECTION
////////////////////////////////////////////////////////////////////////////////////////////////////

// LEN = Length, REL = Relation, THD = Thread.
#define LEN_REL 12
#define FILESTEM "rel12/"	/* Prefix for each output file */
#define N_THDS 16
#define PREFIX_PER_THD 2
#define LEN_PREFIX 3

// Each row is a set of prefixes assigned to a corresponding thread.
// Note: Only list canonical stems.
const char THREAD_STEMS[N_THDS][PREFIX_PER_THD][LEN_PREFIX] = {
	{"UUR", "UUr"},
	{"UUD", "UUd"},
	{"URU", "URu"},
	{"URR", "URF"},
	{"URf", "URB"},
	{"URb", "URL"},
	{"URl", "URD"},
	{"URd", "UrU"},
	{"Uru", "Urr"},
	{"UrF", "Urf"},
	{"UrB", "Urb"},
	{"UrL", "Url"},
	{"UrD", "Urd"},
	{"UDR", "UDr"},
	{"UDD", "UdR"},
	{"Udr", "Udd"},
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// CODE BEGIN
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>

#define FACE(move) (toupper(move)) /* What face is `move` on? */
#define CW(move) (isupper(move)) /* Is `move` clockwise? */
#define INV(move) (move ^ 0x20) /* Invert case for ASCII letters <=> Invert turning chirality */

// A dirty method to get the "axis" of a move, represented by one of the 2 parallel faces (URF).
#define AXIS(move) \
	(FACE(move) == 'D' ? 'U' : FACE(move) == 'L' ? 'R' : FACE(move) == 'B' ? 'F' : FACE(move))

#define MAX(a, b) (a < b ? b : a)

// Moves as permutations of stickers.
// The 48 non-center stickers are numbered.
// For each move, we specify where each sticker will move to, by relative offset.
// I did it like this kinda just for aesthetic reasons. Sorry it's so non-standard lmao
const signed char U[] = {
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
	  0,	  0,	  6,	  6,	  6,	  0,	  0,	  0,
	 20,	 20,	 20,	  0,	  0,	  0,	  0,	  0,
	-12,	  0,	  0,	  0,	  0,	  0,	-20,	-20,
	  0,	  0,	  0,	  0,	 -6,	 -6,	-14,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
};
const signed char R[] = {
	 28,	 28,	 28,	  0,	  0,	  0,	  0,	  0,
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
	  0,	  0,	-18,	-18,	-18,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	 18,	 18,	 10,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	-20,	  0,	  0,	  0,	  0,	  0,	-28,	-28,
};
const signed char F[] = {
	  0,	  0,	  6,	  6,	  6,	  0,	  0,	  0,
	 36,	 36,	 36,	  0,	  0,	  0,	  0,	  0,
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	-28,	  0,	  0,	  0,	  0,	  0,	-36,	-36,
	  0,	  0,	  0,	  0,	 -6,	 -6,	-14,	  0,
};
const signed char B[] = {
	 36,	  0,	  0,	  0,	  0,	  0,	 28,	 28,
	  0,	  0,	  0,	  0,	 -6,	 -6,	-14,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
	  0,	  0,	  6,	  6,	  6,	  0,	  0,	  0,
	-28,	-28,	-28,	  0,	  0,	  0,	  0,	  0,
};
const signed char L[] = {
	  0,	  0,	  0,	  0,	 18,	 18,	 10,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	 28,	  0,	  0,	  0,	  0,	  0,	 20,	 20,
	-20,	-20,	-20,	  0,	  0,	  0,	  0,	  0,
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
	  0,	  0,	-18,	-18,	-18,	  0,	  0,	  0,
};
const signed char D[] = {
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	 20,	  0,	  0,	  0,	  0,	  0,	 12,	 12,
	  0,	  0,	  0,	  0,	 -6,	 -6,	-14,	  0,
	  0,	  0,	  6,	  6,	  6,	  0,	  0,	  0,
	-12,	-12,	-12,	  0,	  0,	  0,	  0,	  0,
	  2,	  2,	  2,	  2,	  2,	  2,	 -6,	 -6,
};
// I = Inverse (counterclockwise)
const signed char UI[] = {
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
	  0,	  0,	 20,	 20,	 12,	  0,	  0,	  0,
	 -6,	 -6,	 -6,	  0,	  0,	  0,	  0,	  0,
	 14,	  0,	  0,	  0,	  0,	  0,	  6,	  6,
	  0,	  0,	  0,	  0,	-20,	-20,	-20,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
};
const signed char RI[] = {
	 18,	 18,	 18,	  0,	  0,	  0,	  0,	  0,
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
	  0,	  0,	 28,	 28,	 20,	  0,	  0,	  0,
	  0,	  0,	  0,	  0,	-28,	-28,	-28,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	-10,	  0,	  0,	  0,	  0,	  0,	-18,	-18,
};
const signed char FI[] = {
	  0,	  0,	 36,	 36,	 28,	  0,	  0,	  0,
	 -6,	 -6,	 -6,	  0,	  0,	  0,	  0,	  0,
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	 14,	  0,	  0,	  0,	  0,	  0,	  6,	  6,
	  0,	  0,	  0,	  0,	-36,	-36,	-36,	  0,
};
const signed char BI[] = {
	 14,	  0,	  0,	  0,	  0,	  0,	  6,	  6,
	  0,	  0,	  0,	  0,	 28,	 28,	 28,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
	  0,	  0,	-28,	-28,	-36,	  0,	  0,	  0,
	 -6,	 -6,	 -6,	  0,	  0,	  0,	  0,	  0,
};
const signed char LI[] = {
	  0,	  0,	  0,	  0,	 20,	 20,	 20,	  0,
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	-10,	  0,	  0,	  0,	  0,	  0,	-18,	-18,
	 18,	 18,	 18,	  0,	  0,	  0,	  0,	  0,
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
	  0,	  0,	-20,	-20,	-28,	  0,	  0,	  0,
};
const signed char DI[] = {
	  0,	  0,	  0,	  0,	  0,	  0,	  0,	  0,
	 14,	  0,	  0,	  0,	  0,	  0,	  6,	  6,
	  0,	  0,	  0,	  0,	 12,	 12,	 12,	  0,
	  0,	  0,	-12,	-12,	-20,	  0,	  0,	  0,
	 -6,	 -6,	 -6,	  0,	  0,	  0,	  0,	  0,
	  6,	  6,	 -2,	 -2,	 -2,	 -2,	 -2,	 -2,
};

// Permutes sticker arrangement `obj` via permutation `perm`, in-place.
// Thought another way, composes two permutations `obj` and `perm`, mutating `obj` to the result.
// `obj` and `perm` are descriptions of the sticker placements, as relative index offsets.
// The identity (solved) case is all 0s.
void permute(signed char obj[48], signed char perm[48]) {
	signed char permuted[48];
	for(int i = 0; i < 48; i++) {
		permuted[i] = obj[i] + perm[i + obj[i]];
	}
	memcpy(obj, permuted, 48);
}

// Return first uncanon index, or -1 if all canon.
int firstUncanon(char *alg, int start) {
	// Discard if nothing to search
	if(alg[start] == '\0')
		return -1;

	// alg[0] must be U
	if(start == 0 && alg[0] != 'U')
		return 0;

	switch(alg[1]) {
		case '\0': // Whole alg can be "U"
			return -1;
		case 'U':
		case 'R':
		case 'r':
		case 'D':
		case 'd':
			break;
		default: // Second move must be canon
			return 1;
	}

	// General case (i=0,1 handled earlier)
	for(int i = MAX(start, 2); alg[i] != '\0'; i++) {
		// Canon second axis
		if(FACE(alg[i]) != 'R') {
			bool ok = false;

			for(int j = i - 1; j >= 0; j--) {
				if(AXIS(alg[j]) != 'U') {
					ok = true;
					break;
				}
			}

			if(!ok)
				return i;
		}

		// No undos
		if(alg[i] == INV(alg[i - 1]))
			return i;

		// Clockwise double-turns
		if(alg[i - 1] == alg[i] && !CW(alg[i]))
			return i;

		// No 3 in a row
		if(alg[i - 2] == alg[i - 1] && alg[i - 1] == alg[i])
			return i;

		// Canon commutative order
		if(
			AXIS(alg[i - 1]) == AXIS(alg[i]) &&
			FACE(alg[i - 1]) != FACE(alg[i]) &&
			AXIS(alg[i]) != FACE(alg[i])
		)
			return i;
	}

	return -1;
}

// TODO: Can avoid this if I just represent moves with numbers 0-11. But meh
char nextMove(char move) {
	switch(move) {
		case 'U':
			return 'u';
		case 'u':
			return 'R';
		case 'R':
			return 'r';
		case 'r':
			return 'F';
		case 'F':
			return 'f';
		case 'f':
			return 'B';
		case 'B':
			return 'b';
		case 'b':
			return 'L';
		case 'L':
			return 'l';
		case 'l':
			return 'D';
		case 'D':
			return 'd';
		case 'd':
			return 'U';
		default:
			return '\0';
	}
}

// Increments the algorithm, in place.
// Sets all trailing un-incremented moves to U.
// Returns the first index of change.
// 	E.g. alg = "URRR"
// 		place = 3 (end)
// 			=> returns "URRr" (last is inc'd)
// 		place = 0 (beginning)
// 			=> returns "uUUU" (first is inc'd, and ALL after become U.)
int incAlg(char *alg, int place) {
	while(alg[place] == 'd' && place > 0) // Carry
		place -= 1;

	alg[place] = nextMove(alg[place]);
	for(int i = place + 1; alg[i] != '\0'; i++)
		alg[i] = 'U';

	return place;
}

// Mutate `alg` to next canon.
// Restricts to algos that start with `prefix`.
// Returns true if success, false if fail (overflow / next algo wouldn't start with `prefix`).
bool mkNextCanon(char *alg, const char prefix[LEN_PREFIX]) {
	int len = strlen(alg);
	incAlg(alg, len - 1);

	if(alg[LEN_PREFIX - 1] != prefix[LEN_PREFIX - 1])
		return false;

	int uncanon = firstUncanon(alg, 0);
	int firstChange;
	while(uncanon != -1) {
		firstChange = incAlg(alg, uncanon);

		if(alg[LEN_PREFIX - 1] != prefix[LEN_PREFIX - 1])
			return false;

		uncanon = firstUncanon(alg, firstChange);
	}

	return true;
}

// TODO: something-something BST or hashmap or whatever
signed char *getPerm(char key) {
	switch(key) {
		case 'U':
			return (signed char*)U;
		case 'u':
			return (signed char*)UI;
		case 'R':
			return (signed char*)R;
		case 'r':
			return (signed char*)RI;
		case 'F':
			return (signed char*)F;
		case 'f':
			return (signed char*)FI;
		case 'B':
			return (signed char*)B;
		case 'b':
			return (signed char*)BI;
		case 'L':
			return (signed char*)L;
		case 'l':
			return (signed char*)LI;
		case 'D':
			return (signed char*)D;
		case 'd':
			return (signed char*)DI;
		default:
			return NULL;
	}
}

// Simulates a cube to see if `alg` is an identity (returns all stickers to starting positions)
bool isId(char *alg) {
	signed char state[48];
	memset(state, 0, 48);
	for(int i = 0; alg[i] != '\0'; i++) {
		permute(state, getPerm(alg[i]));
	}

	for(int i = 0; i < 48; i++)
		if(state[i] != 0)
			return false;
	return true;
}

// Main algorithm.
// Using (void*)s because pthreads
void *findIds(void *prefixes_data) {
	const char (*prefixes)[LEN_PREFIX] = (const char(*)[LEN_PREFIX])prefixes_data;

	char alg[LEN_REL+1];
	alg[LEN_REL] = '\0';

	int len_fstem = strlen(FILESTEM);
	char fname[len_fstem + LEN_PREFIX + 1];
	memcpy(fname, FILESTEM, len_fstem);
	fname[len_fstem + LEN_PREFIX] = '\0';

	for(int i = 0; i < PREFIX_PER_THD; i++) {
		memcpy(fname + len_fstem, prefixes[i], LEN_PREFIX);

		FILE *fin = fopen(fname, "r");
		if(!fin) { // Start with <PREFIX><ALL 'U's>
			memset(alg, 'U', LEN_REL);
			memcpy(alg, prefixes[i], LEN_PREFIX);
		} else { // Start where we left off
			// TODO: A responsible C coder would do more error-checking.
			fseek(fin, 0, SEEK_END);

			if(ftell(fin) < LEN_REL) { // File is probably empty
				memset(alg, 'U', LEN_REL);
				memcpy(alg, prefixes[i], LEN_PREFIX);
			} else {
				// Read the last algorithm into memory.
				fseek(fin, 0, ftell(fin) - LEN_REL - 1);
				fread(alg, LEN_REL, 1, fin);
				fclose(fin);

				if(alg[0] == '-') // A line of --dashes-- indicates a completed search.
					continue;
			}
		}

		// Main loop.
		while(mkNextCanon(alg, prefixes[i])) {
			if(isId(alg)) {
				FILE *fout = fopen(fname, "a");
				fprintf(fout, "%s\n", alg);
				fclose(fout);

				printf("%s\n", alg);
			}
		}

		FILE *fout = fopen(fname, "a");
		for(int j = 0; j < LEN_REL; j++)
			fputc('-', fout); // Search complete, so we put the --dashes--.
		fputc('\n', fout);
		fclose(fout);
	}

	return NULL;
}

// Essentially just thread management.
int main() {
	pthread_t threads[N_THDS];
	for(int i = 0; i < N_THDS; i++)
		pthread_create(&threads[i], NULL, findIds, (void*)THREAD_STEMS[i]);

	for(int i = 0; i < N_THDS; i++)
		pthread_join(threads[i], NULL);

	return 0;
}
