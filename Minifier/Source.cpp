#include <Windows.h>
#include "..\..\..\..\Desktop\Mathis\Code\headers\utils.h"

#define SYNTHAX_TYPE_HTML 1
#define SYNTHAX_TYPE_CSS 2
#define SYNTHAX_TYPE_JS 3

void Exit(DWORD dwExitCode);
void GetErrorString(LPSTR lpszError);
bool IsBadChar(char ch);

int main(int argc, char* argv[]) {

	SetConsoleOutputCP(CP_UTF7);

	if (argc != 6) {
		puts("[html/css/js] -in \"Fichier à optimiser.css\" -out \"Fichier optimisé.css\"");
		Exit(1);
	}

	byte synthaxType = 0;
	char szInputFile[PATH_BUFFER_SIZE];
	char szOutputFile[PATH_BUFFER_SIZE];
	char szArgLower[5];
	memcpy(szArgLower, argv[1], 5);
	ToLower(szArgLower);
	if (Equal(szArgLower, "html")) {
		synthaxType = SYNTHAX_TYPE_HTML;
	}
	else if (Equal(szArgLower, "css")) {
		synthaxType = SYNTHAX_TYPE_CSS;
	}
	else if (Equal(szArgLower, "js")) {
		synthaxType = SYNTHAX_TYPE_JS;
	}
	else {
		puts("Langage non spécifié.");
		Exit(-1);
	}

	for (int i = 2; i < argc; i++) {
		memcpy(szArgLower, argv[i], 5);
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
		printf("argc = %d\nargv[%d] = %s\n", argc, i, argv[i]);
		Exit(-1);
	}

	HANDLE hInputFile = CreateFileA(szInputFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hInputFile == INVALID_HANDLE_VALUE) {
		GetErrorString(szInputFile);
		printf("Erreur lors de l'ouverture du fichier à optimiser: %s\n", szInputFile);
		Exit(2);
	}

	HANDLE hOutputFile = CreateFileA(szOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
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
	char lastChar;		// Garde une trace du dernier caractère écrit

	for (DWORD i = 0; i < dwFileSize; i++) {
		if (lpFileBuffer[i] != '\n' && lpFileBuffer[i] != '\r') {
			switch (synthaxType) {
			case SYNTHAX_TYPE_HTML: {

				lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
				lastChar = lpOutFileBuffer[dwOutIndex];
				dwOutIndex++;


				break;
			}
		parseCSS:
			case SYNTHAX_TYPE_CSS: {

				
				if (IsBadChar(lpFileBuffer[i])) {
					DWORD dwSpaces = 1;
					while (IsBadChar(lpFileBuffer[i + dwSpaces]))
						dwSpaces++;
					register char after = lpFileBuffer[i + dwSpaces];
					if (after == '{' || after == ':' || after == ';' || after == ',' || after == '/') {		// Supprime les espaces avant
						i += dwSpaces;
					}
					else if (dwSpaces > 1) {	// Remplace les suites de plusieurs espaces par un seul
						i += dwSpaces - 1;
						lpFileBuffer[i] = ' ';
					}
				}


				if (lpFileBuffer[i] == '/') {

					if (lpFileBuffer[i + 1] == '*') {
						i += 2;
						while (lpFileBuffer[i] != '*' || lpFileBuffer[i + 1] != '/')	// Skip les commentaires
							i++;
						i += 2;
						while (IsBadChar(lpFileBuffer[i])) i++;
						goto parseCSS;
					}
				}


				if (lpFileBuffer[i] == '}') {
					if (lastChar == ';') {
						dwOutIndex--;
					}
					else if (lastChar == '{') {
						i++;
						while (lpOutFileBuffer[dwOutIndex != 0 ? dwOutIndex - 1 : 0] != '}' && dwOutIndex != 0) {	// Supprime les règles vides
							dwOutIndex--;
						}
						goto parseCSS;
					}
				}


				lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
				lastChar = lpOutFileBuffer[dwOutIndex];
				dwOutIndex++;

				if (lpFileBuffer[i] == '{' || lpFileBuffer[i] == '}' || lpFileBuffer[i] == ':' || lpFileBuffer[i] == ';' || lpFileBuffer[i] == ',' || lpFileBuffer[i] == '/') {
					while (IsBadChar(lpFileBuffer[i + 1])) i++;
					/*if (lpFileBuffer[i] == '/' && lpFileBuffer[i + 1] == '*') {
						i += 2;
						while (lpFileBuffer[i] != '*' || lpFileBuffer[i + 1] != '/')	// Skip les commentaires
							i++;
						i += 2;
						while (IsBadChar(lpFileBuffer[i + 1])) i++;
						
					}*/
				}

				
				break;
			}
			case SYNTHAX_TYPE_JS: {

				lpOutFileBuffer[dwOutIndex] = lpFileBuffer[i];
				lastChar = lpOutFileBuffer[dwOutIndex];
				dwOutIndex++;

				break;
			}
			}
			
			continue;
		}
	}

	LPSTR lpOutFilePtr = lpOutFileBuffer;
	if (*lpOutFileBuffer == ' ') {	// Supprime l'espace au début du fichier
		lpOutFilePtr++;
		dwOutIndex--;
	}

	if (!WriteFile(hOutputFile, lpOutFilePtr, dwOutIndex, &dw, NULL)) {
		GetErrorString(szInputFile);
		printf("Erreur lors de la lecture du fichier à optimiser: %s\n", szInputFile);
		Exit(7);
	}

	free(lpFileBuffer);
	free(lpOutFileBuffer);
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