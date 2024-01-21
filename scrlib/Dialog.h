static const int DialogHOME	= -8;
static const int DialogEND	= -7;
static const int DialogRIGHT	= -6;
static const int DialogLEFT	= -5;
static const int DialogUP	= -4;
static const int DialogDOWN	= -3;
static const int DialogPREV	= -2;
static const int DialogNEXT	= -1;
static const int DialogCANCEL	= 0;
static const int DialogOK	= 9999;

class DialogElement {
public:
	int r, c;
	int w, h;
	int oblig;
	static int baserow, basecol;
	DialogElement *next, *prev;
	static DialogElement *list;
	static unsigned char *palette;
	static Screen *scr;

	DialogElement (int row, int col, int reg=0);
	virtual ~DialogElement () {}
	virtual void Draw ()
		= 0;
	virtual void Redraw ()
		{}
	virtual int Run ()
		{ return DialogNEXT; }
	DialogElement *Find (int dir);
	DialogElement *Next ()
		{ return next==list ? 0 : next; }
	static DialogElement *Home ();
	static DialogElement *End ();
	static char *GetString (char **p);
	static long GetNumber (char **p);
};

class DialogGroup : public DialogElement {
	DialogElement **tab;
	int nv;
	int maxnv;
public:
	int frameflag;

	DialogGroup (int row, int col, char **fmt, void ***arg,
		int flags=0, DialogGroup *prnt=0);
	~DialogGroup ();
	void Add (DialogElement *v);
	void Center (int wid=0);
	void Draw ();
};

class DialogLabel : public DialogElement {
	char *label;
public:
	DialogLabel (int row, int col, char **p);
	~DialogLabel () { if (label) delete label; }
	void Draw ();
};

class DialogLine : public DialogElement {
	DialogGroup *parent;
public:
	DialogLine (int row, int col, DialogGroup *p)
		: DialogElement (row, col)
		{ h = 1; parent = p; }
	void Draw ();
};

class DialogButton : public DialogElement {
	char *label;
	int *val;
public:
	DialogButton (int row, int col, char **p, int *v);
	~DialogButton () { if (label) delete label; }
	void Draw ();
	int Run ();
};

class DialogRadioButton : public DialogElement {
	int *val;
	int setval;
	char *label;
public:
	DialogRadioButton (int row, int col, char **p, int *v);
	~DialogRadioButton () { if (label) delete label; }
	void Draw ();
	void Redraw ();
	int Run ();
};

class DialogList : public DialogElement {
	char str[80];
	long *val;
	int maxw;
	int (*func) (Screen *scr, int y, int x, int w, long *v, char *str);
public:
	DialogList (int row, int col, char **p, long *v, int (*)(...));
	void Draw ();
	int Run ();
};

class DialogString : public DialogElement {
	char *val;
	int maxlen;
public:
	DialogString (int row, int col, char **p, char *v);
	void Draw ();
	int Run ();
};

class DialogTitle : public DialogElement {
	char *val;
public:
	DialogTitle (int row, int col, char **p, char *v);
	void Draw ();
};

class DialogPassword : public DialogElement {
	char *val;
	int maxlen;
public:
	DialogPassword (int row, int col, char **p, char *v);
	void Draw ();
	int Run ();
};

class DialogNumber : public DialogElement {
	long *val;
	long min, max, step;
public:
	DialogNumber (int row, int col, char **p, long *v);
	void Draw ();
	int Run ();
};

class DialogMask : public DialogElement {
	long *val;
public:
	DialogMask (int row, int col, char **p, long *v);
	void Draw ();
	int Run ();
};

class DialogCommand : public DialogElement {
	char *label;
	int val;
public:
	DialogCommand (int row, int col, char **p);
	~DialogCommand () { if (label) delete label; }
	void Draw ();
	int Run ();
};

class Dialog {
	char *title;
	unsigned char *palette;
	int r, c;
	DialogGroup *grp;
	DialogElement *list;
public:
	Dialog (char *t, unsigned char *pal, char *fmt, ...);
	~Dialog () { if (grp) delete grp; }
	int Run (Screen *scr);
};
