#include <string.h>
#include "Screen.h"

extern Screen V;
extern int TextColor;
extern int HelpColor;

struct helptab {
	const char *item;
	const char *rinfo;
} helpTab [] = {
{ "usage",
	"Вызов:\n"
	"        diag [-e | -r] [-a]\n"
	"\n"
	"Флаги:\n"
	"        -e     - английская диагностика\n"
	"        -r     - русская диагностика\n"
	"        -a     - выкючение автоконфигурации\n",
},
{0} };

void Help (char *item)
{
	// Выдача справки по указанной теме.
	struct helptab *p = helpTab;
	for (; p->item; ++p) {
		if (! strcmp (item, p->item)) {
			break;
                }
        }
	V.PopupString (" Справка ", p->item ? p->rinfo : "Нет информации\n",
		" Готово ", HelpColor, TextColor);
}
