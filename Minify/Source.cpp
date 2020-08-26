#include <Windows.h>
#include "utils.h"

#define LANGAGE_HTML 1
#define LANGAGE_CSS 2
#define LANGAGE_JS 3

void Exit(DWORD dwExitCode);
void GetErrorString(LPSTR lpszError);
bool IsBadChar(char ch);


int main(int argc, char* argv[]) {

	SetConsoleOutputCP(CP_UTF7);

	if (argc < 6) {
		if (argc == 2) {
			if (Equal(argv[1], "--version")) {
				puts(__DATE__);
				Exit(0);
			}
		}
		printf("%s [html/css/js] -in input_file -out output_file\n", argv[0]);
		Exit(1);
	}


	byte langage = 0;
	bool bQuietMode = false;
	char szInputFile[PATH_BUFFER_SIZE];
	char szOutputFile[PATH_BUFFER_SIZE];
	char szArgLower[6];
	memcpy(szArgLower, argv[1], 5);
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
		puts("Langage non spécifié.");
		Exit(-1);
	}

	for (int i = 2; i < argc; i++) {
		memcpy(szArgLower, argv[i], 6);
		ToLower(szArgLower);

		if (Equal(szArgLower, "-in")) {
			i++;
			strcpy(szInputFile, argv[i]);
			continue;
		}
		if (Equal(szArgLower, "-out")) {
			i++;
			strcpy(szOutputFile, argv[i]);
			continue;
		}
		if (Equal(szArgLower, "quiet")) {
			bQuietMode = true;
			continue;
		}
		printf("argc = %d\nargv[%d] = %s\n", argc, i, argv[i]);
		Exit(-1);
	}

	HANDLE hInputFile = CreateFileA(szInputFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInputFile == INVALID_HANDLE_VALUE) {
		GetErrorString(szInputFile);
		printf("Erreur lors de l'ouverture du fichier à optimiser: %s\n", szInputFile);
		Exit(2);
	}

	HANDLE hOutputFile = CreateFileA(szOutputFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOutputFile == INVALID_HANDLE_VALUE) {
		GetErrorString(szInputFile);
		printf("Erreur lors de la création du fichier optimisé: %s\n", szInputFile);
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
		printf("Erreur lors de la lecture du fichier à optimiser: %s\n", szInputFile);
		Exit(5);
	}
	LPSTR lpOutFileBuffer = (LPSTR)malloc(dwFileSize);
	if (lpOutFileBuffer == NULL) {
		puts("Erreur lors de l'allocation de la mémoire.");
		Exit(6);
	}
	ReplaceAllChars(lpFileBuffer, '\t', ' ');
	DWORD dwOutIndex = 0;
	char lastChar = 0;		// Garde une trace du dernier caractère écrit

	if (langage == LANGAGE_CSS) goto CSS; else if (langage == LANGAGE_JS) goto JS;


HTML:
	char szLastTag[11];
	bool inTag, inCloseTag, inQuotes, inDoubleQuotes;
	inTag = false;
	inCloseTag = false;
	inQuotes = false;
	inDoubleQuotes = false;


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
	for (DWORD i = 0; i < dwFileSize; i++) {
	CSS_loop:

		if (IsBadChar(lpFileBuffer[i])) {
			DWORD dwSpaces = 1;
			while (IsBadChar(lpFileBuffer[i + dwSpaces])) {
				dwSpaces++;
			}
			if (lpFileBuffer[i + dwSpaces] == '/' || lpFileBuffer[i + dwSpaces + 1] == '*') {	// Supprime les commentaires récursivement
				i += dwSpaces;
				goto CSS_parse_comment;
			}
			register char next = lpFileBuffer[i + dwSpaces];
			if (next == '{' || next == '}' || next == '(' || next == ')' || next == ']' || next == '=' || next == ':' || next == ';' || next == ',' || next == '\'' || next == '\"' || (lastChar == ':' || lastChar == ';' && next != ' ')) {		// Supprime les espaces avant ces caractères
				if (dwOutIndex && lastChar == ' ') dwOutIndex--;
				i += dwSpaces;
			}
			else if (dwSpaces > 1) {	// Remplace les suites de plusieurs espaces par un seul
				i += dwSpaces - 1;
				lpFileBuffer[i] = ' ';
			}
		}
	CSS_parse_comment:
		
		if (lpFileBuffer[i] == '/') {

			if (lpFileBuffer[i + 1] == '*') {
				i += 2;
				while (lpFileBuffer[i] != '*' || lpFileBuffer[i + 1] != '/' && i < dwFileSize)	// Skip les commentaires
					i++;
				i += 2;
				goto CSS_loop;
			}
		}

		if (lpFileBuffer[i] == '}') {
			if (lastChar == ';') {
				dwOutIndex--;
			}
			else if (lastChar == '{') {		// Supprime les règles vides
				i++;
				while (lpOutFileBuffer[dwOutIndex ? dwOutIndex - 1 : 0] != '}' && dwOutIndex) {
					dwOutIndex--;
				}
				goto CSS_loop;
			}
		}

		if (lastChar == '{' || lastChar == '}' || lastChar == '[' || lastChar == '(') {
			while (IsBadChar(lpFileBuffer[i])) i++;
		}
		
		if (lastChar == ' ') {
			if (lpFileBuffer[i] == ' ') {
				i++;
				goto CSS_loop;
			}
			if (lpFileBuffer[i] == '{' || lpFileBuffer[i] == ':' || lpFileBuffer[i] == ';' || lpFileBuffer[i] == ',') {
				dwOutIndex--;
			}
		}

		if (i >= dwFileSize) {
			goto end;
		}

		lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
		lastChar = lpOutFileBuffer[dwOutIndex];
		dwOutIndex++;
		
		register char next = lpFileBuffer[i];
		if (next == '{' || next == '}' || next == '(' || next == '[' || next == '=' || next == ':' || next == ';' || next == ',' || next == '\'' || next == '\"') {
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
	if (*lpOutFileBuffer == ' ') {	// Supprime l'espace au début du fichier
		lpOutFilePtr++;
	}
	
	if (!WriteFile(hOutputFile, lpOutFilePtr, dwOutIndex, &dw, NULL)) {
		GetErrorString(szInputFile);
		printf("WriteFile(hOutputFile, %lu): %s\n", dwOutIndex, szInputFile);
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

void GetErrorString(LPSTR lpszError) {
	DWORD dwError = GetLastError();
	DWORD dwMessageLength = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, LANG_USER_DEFAULT, lpszError, 261, NULL);
	if (dwMessageLength == 0) {
		_ultoa(dwError, lpszError, 10);
	}
	return;
}

void Exit(DWORD dwExitCode) {
	ExitProcess(dwExitCode);
}
