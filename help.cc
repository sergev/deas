#include <string.h>
#include "Screen.h"

extern Screen V;
extern int TextColor;
extern int HelpColor;

struct helptab {
	char *item;
	char *rinfo;
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
	for (struct helptab *p=helpTab; p->item; ++p)
		if (! strcmp (item, p->item))
			break;
	V.PopupString (" Справка ", p->item ? p->rinfo : "Нет информации\n",
		" Готово ", HelpColor, TextColor);
}
