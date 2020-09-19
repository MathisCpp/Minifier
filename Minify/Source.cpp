#include <Windows.h>
#include "..\..\..\..\Desktop\Mathis\Code\headers\utils.h"

#define LANGAGE_HTML 1
#define LANGAGE_CSS 2
#define LANGAGE_JS 3

void Exit(DWORD dwExitCode);
void PrintHelp(char* szProgramName);
void GetErrorString(LPSTR lpszError);
bool IsBadChar(char ch);
bool IsSpecialChar(char ch);

const char specialChars[] = { '\'','\"',',',';',':','{','}','(',')','[',']',' ','#','!','/' };


int main(int argc, char* argv[]) {

	SetConsoleOutputCP(CP_UTF7);

	if (argc == 1) {
		PrintHelp(argv[0]);
	}
	else if (argc == 2) {
		ToLower(argv[1]);
		if (Equal(argv[1], "--version")) {
			puts(__DATE__);
			Exit(0);
		}
		if (Equal(argv[1], "--help")) {
			PrintHelp(argv[0]);
		}
	}
	
	


	byte langage = 0;
	bool bQuietMode = false, inFileSet = false, outFileSet = false;
	char szInputFile[PATH_BUFFER_SIZE];
	char szOutputFile[PATH_BUFFER_SIZE];
	
	


	for (int i = 1; i < argc; i++) {
		char szArgLower[11];
		memcpy(szArgLower, argv[i], 11);
		ToLower(szArgLower);

		if (Equal(szArgLower, "--language")) {
			if (langage) {
				puts("Langage déjà spécifié.");
				Exit(-1);
			}

			if (++i == argc) {
				break;
			}
			memcpy(szArgLower, argv[i], 5);
			ToLower(szArgLower);
			if (Equal(szArgLower, "html")) {
				langage = LANGAGE_HTML;
			}
			else if (Equal(szArgLower, "css")) {
				langage = LANGAGE_CSS;
			}
			else if (Equal(szArgLower, "js")) {
				langage = LANGAGE_JS;
			}
			else {
				puts("Langage non valide.");
				Exit(-1);
			}
			continue;
		}
		if (Equal(szArgLower, "--in")) {
			if (inFileSet) {
				puts("Fichier d'entrée déjà spécifié.");
				Exit(1);
			}

			if (++i == argc) {
				break;
			}
			strcpy(szInputFile, argv[i]);
			inFileSet = true;
			continue;
		}
		if (Equal(szArgLower, "--out")) {
			if (outFileSet) {
				puts("Fichier de sortie déjà spécifié.");
				Exit(1);
			}

			if (++i == argc) {
				break;
			}
			strcpy(szOutputFile, argv[i]);
			outFileSet = true;
			continue;
		}
		if (Equal(szArgLower, "--quiet")) {
			if (bQuietMode) {
				puts("Mode silencieux déjà activé.");
				Exit(1);
			}

			bQuietMode = true;
			continue;
		}
		printf("Argument non reconnu: \"%s\"\n", argv[i]);
		Exit(-1);
	}

	if (langage == 0) {
		puts("Langage non spécifié.");
		Exit(1);
	}
	else if (!inFileSet) {
		puts("Fichier d'entrée non spécifié.");
		Exit(1);
	}
	else if (!outFileSet) {
		puts("Fichier de sortie non spécifié.");
		Exit(1);
	}

	HANDLE hInputFile = CreateFileA(szInputFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInputFile == INVALID_HANDLE_VALUE) {
		GetErrorString(szInputFile);
		printf("Erreur lors de l'ouverture du fichier source.\n%s", szInputFile);
		Exit(2);
	}

	HANDLE hOutputFile = CreateFileA(szOutputFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOutputFile == INVALID_HANDLE_VALUE) {
		GetErrorString(szInputFile);
		printf("Erreur lors de la création du fichier de destination.\n%s", szInputFile);
		Exit(2);
	}

	LARGE_INTEGER li;
	GetFileSizeEx(hInputFile, &li);
	if (li.QuadPart > 104857600) {
		puts("La taille du fichier source ne doit pas dépasser 100Mo.");
		Exit(3);
	}
	DWORD dw, dwFileSize = li.QuadPart;
	LPSTR lpFileBuffer = (LPSTR)malloc(dwFileSize);
	if (lpFileBuffer == NULL) {
		puts("Erreur lors de l'allocation de la mémoire.");
		Exit(4);
	}
	if (!ReadFile(hInputFile, lpFileBuffer, dwFileSize, &dw, NULL)) {
		GetErrorString(szInputFile);
		printf("Erreur lors de la lecture du fichier source: %s\n", szInputFile);
		Exit(5);
	}
	LPSTR lpOutFileBuffer = (LPSTR)malloc(dwFileSize);
	if (lpOutFileBuffer == NULL) {
		puts("Erreur lors de l'allocation de la mémoire.");
		Exit(6);
	}
	ReplaceAllChars(lpFileBuffer, '\t', ' ');
	DWORD dwOutIndex = 0, dwSkipCharsCount = 0, dwSkipCharsBeginIndex = -1;
	char lastChar = 0;		// Garde une trace du dernier caractère écrit
	bool inQuotes = false, inDoubleQuotes = false;
	

	if (langage == LANGAGE_CSS) goto CSS; else if (langage == LANGAGE_JS) goto JS;


HTML:
	char szLastTag[11];
	bool inTag, inCloseTag;
	inTag = false;
	inCloseTag = false;


	for (DWORD i = 0; i < dwFileSize; i++) {
	HTML_loop:

		if (IsBadChar(lpFileBuffer[i])) {
			DWORD dwSpaces = 1;
			while (IsBadChar(lpFileBuffer[i + dwSpaces])) {
				dwSpaces++;
			}
			if (Equal(lpFileBuffer + i + dwSpaces, "<!--", 4)) {	// Supprime les commentaires récursivement
				i += dwSpaces;
				goto HTML_parse_comment;
			}
			register char next = lpFileBuffer[i + dwSpaces];
			if (next == '<' || next == '>' || next == '/') {		// Supprime les espaces avant ces caractères
				if (dwOutIndex && lastChar == ' ') dwOutIndex--;
				i += dwSpaces;
			}
			else if (dwSpaces > 1) {	// Remplace les suites de plusieurs espaces par un seul
				i += dwSpaces - 1;
				lpFileBuffer[i] = ' ';
			}
		}

	HTML_parse_comment:

		if (Equal(lpFileBuffer + i, "<!--", 4)) {

			i += 4;
			while (!Equal(lpFileBuffer + i, "-->", 3) && i < dwFileSize)	// Skip les commentaires
				i++;
			i += 3;
			goto HTML_loop;
			
		}

		if (lpFileBuffer[i] == '<') {
			if (inTag || inCloseTag) {
				puts("Erreur: ouverture d'une balise dans uune autre déjà ouverte.");
				Exit(502);
			}
			if (lpFileBuffer[i + 1] != '/') {
				byte lastTagIndex = 0;
				while (lpFileBuffer[i + lastTagIndex + 1] != ' ' && lpFileBuffer[i + lastTagIndex + 1] != '>') {
					if (lastTagIndex >= 11) {
						printf("Tag HTML trop long: %.11s\n", szLastTag);
						Exit(500);
					}

					szLastTag[lastTagIndex++] = lpFileBuffer[i + lastTagIndex + 1];
				}
				szLastTag[lastTagIndex] = 0;
				puts(szLastTag);
				inTag = true;

				if (Equal(szLastTag, "style")) {
					// Récupérer le style, compter le nombre de caractères et le minifier
				}
			}
			else
				inCloseTag = true;
		}
		

		if (lpFileBuffer[i] == '>') {
			if (!inTag && !inCloseTag) {
				printf("Erreur: fermeture de balise alors qu'aucune est ouverte.");
				Exit(501);
			}
			inTag = false;
			inCloseTag = false;
			goto HTML_loop;
		}

		if (lpFileBuffer[i] == '\'') {
			inQuotes = !inQuotes;
		}
		else if (lpFileBuffer[i] == '\"') {
			inDoubleQuotes = !inDoubleQuotes;
		}

		if (i >= dwFileSize) {
			goto end;
		}

		lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
		lastChar = lpOutFileBuffer[dwOutIndex];
		dwOutIndex++;

	}
	goto end;

CSS:
	bool inCSSRule;
	inCSSRule = false;
	

	for (DWORD i = 0; i < dwFileSize; i++) {
	CSS_loop_begin:

		if (i == dwSkipCharsBeginIndex) {
			i += dwSkipCharsCount;
				
			while (dwSkipCharsCount) {
				dwSkipCharsCount--;
			}
			dwSkipCharsBeginIndex = -1;
		}

		if (IsBadChar(lpFileBuffer[i])) {
		
			DWORD dwSpaces = 1;
			while (IsBadChar(lpFileBuffer[i + dwSpaces])) {
				dwSpaces++;
			}

			bool comment = false;
			if (lpFileBuffer[i + dwSpaces] == '/' && lpFileBuffer[i + dwSpaces + 1] == '*') {
				comment = true;
			}

			bool loopFlag = true, isLastCharSpecial = false, isCurrentCharSpecial = false;
			for (byte a = 0; a < sizeof specialChars; a++) {
				if (lastChar == specialChars[a]) {
					isLastCharSpecial = true;
				}
				if (lpFileBuffer[i + dwSpaces] == specialChars[a]) {
					isCurrentCharSpecial = true;
				}
			}

			bool fixedIdentifier = false;
			if (!isLastCharSpecial && !isCurrentCharSpecial && !inCSSRule) {
				DWORD dwCommentIndex = i + 1;
				do {
					if (IsBadChar(lpFileBuffer[dwCommentIndex]))
						dwCommentIndex++;
					else if (lpFileBuffer[dwCommentIndex] == '/' && lpFileBuffer[dwCommentIndex + 1] == '*') {
						i += dwSpaces;
						goto CSS_parse_comment;
					}
					else {
						lpOutFileBuffer[dwOutIndex] = ' ';
						dwOutIndex++;
						lastChar = ' ';
						i += dwSpaces;
						loopFlag = false;
						fixedIdentifier = true;
						dwSpaces = 0;
					}
				} while (loopFlag);
			}

			if (comment) {
				i += dwSpaces;
				goto CSS_parse_comment;
			}

			register char next = lpFileBuffer[i + dwSpaces];
			if (next == '{' || next == '}' || next == '(' || next == ')' || next == ']' || next == '=' || next == ':' || next == ';' || next == ',' || next == '\"' || next == '\'' || (lastChar == ':' || lastChar == ';' && next != ' ')) {		// Supprime les espaces avant ces caractères
				if (dwOutIndex && lastChar == ' ') dwOutIndex--;
				i += dwSpaces;
			}
		}
	CSS_parse_comment:
		
		if (lpFileBuffer[i] == '/') {

			if (lpFileBuffer[i + 1] == '*') {
				i += 2;
				while (lpFileBuffer[i] != '*' || lpFileBuffer[i + 1] != '/') {	// Skip les commentaires
					if (i >= dwFileSize) {
						puts("[CSS][Erreur] Terminaison de commentaire attendue.");
						Exit(-1);
					}
					i++;
				}
				i += 2;
				goto CSS_loop_begin;	// Supprime les espaces/newlines après le commentaire
			}
		}

		if (!IsSpecialChar(lpFileBuffer[i]) && !IsSpecialChar(lastChar) && !inCSSRule && lpFileBuffer[i ? i - 1 : i] != lastChar) {
			lpOutFileBuffer[dwOutIndex] = ' ';
			lastChar = ' ';
			dwOutIndex++;
		}

		else if (lpFileBuffer[i] == '}') {

			if (!inCSSRule) {
				puts("[CSS][Erreur] Fermeture d'une règle alors qu'aucune n'est ouverte.");
				Exit(1002);
			}
			inCSSRule = false;

			if (lastChar == ';') {
				dwOutIndex--;
			}
			else if (lastChar == '{') {		// Supprime les règles vides
				i++;
				while (lpOutFileBuffer[dwOutIndex ? dwOutIndex - 1 : 0] != '}' && dwOutIndex) {
					dwOutIndex--;
				}
				lastChar = '}';
				goto CSS_loop_begin;
			}
		}
		
		else if (lpFileBuffer[i] == '{') {
			if (inCSSRule) {
				puts("[CSS][Erreur] Ouverture d'une règle dans une règle.");
				Exit(1001);
			}
			inCSSRule = true;
		}

		else if (lpFileBuffer[i] == '\"' || lpFileBuffer[i] == '\'') {
			char quoteType = lpFileBuffer[i];
			do {
				if (i >= dwFileSize) {
					goto end;
				}
				lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
				lastChar = lpOutFileBuffer[dwOutIndex];
				dwOutIndex++;
				i++;
			} while (lpFileBuffer[i] != quoteType);
		}

		if (lastChar == '{' || lastChar == '}' || lastChar == '[' || lastChar == '(') {
			while (IsBadChar(lpFileBuffer[i])) i++;
		}
		
		else if (lastChar == ' ') {
			if (lpFileBuffer[i] == ' ') {
				i++;
				goto CSS_loop_begin;
			}
			if (lpFileBuffer[i] == '{' || lpFileBuffer[i] == ':' || lpFileBuffer[i] == ';' || lpFileBuffer[i] == ',') {
				dwOutIndex--;
			}
		}

		else if (lastChar == ':' && inCSSRule) {
			
			if (lpFileBuffer[i] == '0') {
				DWORD dwZeros = 1, dwNumbers = 1;
				
				while (IsNumber(lpFileBuffer[i + dwNumbers])) {	// Compte le nombre de '0' consécutifs
					if (lpFileBuffer[i + dwZeros] == '0') {
						dwZeros++;
					}
					dwNumbers++;
				}
				
				if (!IsNumber(lpFileBuffer[i + dwZeros])) {
					if (lpFileBuffer[i + dwNumbers] == '.') {
						DWORD dwDecimalsAfter = dwNumbers + 1;
						bool removeDot = false;

						while (IsNumber(lpFileBuffer[i + dwDecimalsAfter])) {
							if (lpFileBuffer[i + dwDecimalsAfter] != '0') {
								removeDot = false;
							}
							dwDecimalsAfter++;
						}
						
						dwZeros -= removeDot;	// Enlève les 0 devant les points
					}
					else dwZeros--;		// Décrémentation du compteur pour laisser au moins un 0
				}
				
				if (dwZeros > 1 || lpFileBuffer[i + dwZeros] == '.' || IsNumber(lpFileBuffer[i + dwZeros])) {	// Supprime les suites de plusieurs zéros inutiles
					i += dwZeros;
				}
			}
		}
		
		else if (lastChar == '.' && inCSSRule) {

			int dwNumbersAfter = 0;
			while (IsNumber(lpFileBuffer[i + dwNumbersAfter])) {
				if (lpFileBuffer[i + dwNumbersAfter] != '0') {

				}
				dwNumbersAfter++;
			}
			if (dwNumbersAfter == 0) {
				puts("[CSS][Erreur] Absence de chiffres après un point.");
				Exit(-1);
			}

			dwNumbersAfter--;
			while (lpFileBuffer[i + dwNumbersAfter] == '0') {
				dwSkipCharsCount++;
				dwNumbersAfter--;
			}
			
			if (dwNumbersAfter <= -1) {
				
				i += dwSkipCharsCount;
				
				lpOutFileBuffer[dwOutIndex - 1] = '0';

				lastChar = '0';
				goto CSS_loop_begin;
			}
			dwNumbersAfter++;
			dwSkipCharsBeginIndex = i + dwNumbersAfter;
			
		}

		else if (lpFileBuffer[i] == '.' && !inCSSRule) {
			if (lastChar != '}' && lastChar != 0) {
				puts("[CSS][Erreur] Espace avant un identificateur de classe.");
				Exit(20);
			}
		}

		if (i >= dwFileSize) {
			goto end;
		}

		lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
		lastChar = lpOutFileBuffer[dwOutIndex];
		dwOutIndex++;
		
		
		register char next = lpFileBuffer[i];
		if (next == '{' || next == '}' || next == '(' || next == '[' || next == '=' || next == ':' || next == ';' || next == ',') {
			while (IsBadChar(lpFileBuffer[i + 1]) || lpFileBuffer[i] == '/') {

				if (lpFileBuffer[i] == '/' && lpFileBuffer[i + 1] == '*') {
					i += 2;
					while (lpFileBuffer[i] != '*' || lpFileBuffer[i + 1] != '/' && i < dwFileSize)
						i++;
					i += 2;
				}
				i++;
			}
		}
	}
	goto end;
	

JS:
	for (DWORD i = 0; i < dwFileSize; i++) {
	JS_loop:
		if (lpFileBuffer[i] == '\n' || lpFileBuffer[i] == '\r') continue;

		lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
		lastChar = lpOutFileBuffer[dwOutIndex];
		dwOutIndex++;

	}

	

end:

	free(lpFileBuffer);

	LPSTR lpOutFilePtr = lpOutFileBuffer;
	if (*lpOutFileBuffer == ' ' && dwOutIndex > 0) {	// Supprime l'espace au début du fichier
		puts("*lpOutFileBuffer == ' '");
		lpOutFilePtr++;
		dwOutIndex--;
	}
	
	if (!WriteFile(hOutputFile, lpOutFilePtr, dwOutIndex, &dw, NULL)) {
		GetErrorString(szInputFile);
		printf("Erreur lors de l'écriture du fichier de destination.\n%s", szInputFile);
		Exit(7);
	}

	free(lpOutFileBuffer);
	if (!bQuietMode)
		printf("Avant: %lu\nAprès:  %lu\n%lu octets supprimés.\n", dwFileSize, dwOutIndex, dwFileSize - dwOutIndex);
	Exit(0);
}

bool IsBadChar(char ch) {
	return (ch == ' ' || ch == '\r' || ch == '\n');
}

bool IsSpecialChar(char ch) {
	
	for (byte a = 0; a < sizeof specialChars; a++) {
		if (ch == specialChars[a]) {
			return true;
		}
	}
	return false;
}

void GetErrorString(LPSTR lpszError) {
	DWORD dwError = GetLastError();
	DWORD dwMessageLength = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, LANG_USER_DEFAULT, lpszError, 261, NULL);
	if (!dwMessageLength) {
		_ultoa(dwError, lpszError, 10);
	}
	return;
}

void PrintHelp(char* szProgramName) {
	printf("%s --langage [html/css/js] --in input_file --out output_file\n", szProgramName);
	Exit(1);
}

void Exit(DWORD dwExitCode) {
	ExitProcess(dwExitCode);
}