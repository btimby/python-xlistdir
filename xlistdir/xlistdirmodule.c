#include "Python.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#if defined(__WATCOMC__) && !defined(__QNX__)
#include <direct.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#endif
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#include <process.h>
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#ifdef MS_WIN32
#define popen	_popen
#define pclose	_pclose
#else /* 16-bit Windows */
#include <dos.h>
#include <ctype.h>
#endif /* MS_WIN32 */
#endif /* _MSC_VER */

#if defined(MS_WIN32) && !defined(HAVE_OPENDIR)
/**** ms_win32 version ****/

static PyObject *
win32_error(char* function, char* filename)
{
	/* XXX We should pass the function name along in the future.
	   (_winreg.c also wants to pass the function name.)
	   This would however require an additional param to the 
	   Windows error object, which is non-trivial.
	*/
	errno = GetLastError();
	if (filename)
		return PyErr_SetFromWindowsErrWithFilename(errno, filename);
	else
		return PyErr_SetFromWindowsErr(errno);
}


typedef struct {
	PyObject_HEAD
	PyObject *dirname;
	int itemno;

	/* platform-specific fields */
	HANDLE hFindFile;
	WIN32_FIND_DATA FileData;

	/* MAX_PATH characters could mean a bigger encoded string */
	char namebuf[MAX_PATH*2+5];

} PyXListdirObject;

static PyObject *
xlistdir_opendir(PyXListdirObject *xldo)
{
    /* TODO: Set up xldo fields for the iteration.
     *       This will usually involve interpreting xldo->dirname
     *         and converting it to a format that the os can digest.
     */	
	HANDLE hFindFile;
	WIN32_FIND_DATA *pFileData = &(xldo->FileData);
	char *bufptr = xldo->namebuf;
	char *namebuf = xldo->namebuf;
	int len = sizeof(xldo->namebuf)/sizeof(xldo->namebuf[0]);
	char ch;

	/** XXX do the unicode string thing
	if (!PyArg_ParseTuple(args, "et#:listdir", 
	                      Py_FileSystemDefaultEncoding, &bufptr, &len))
		return NULL;
	 */
	ch = namebuf[len-1];
	if (ch != '/' && ch != '\\' && ch != ':')
		namebuf[len++] = '/';
	strcpy(namebuf + len, "*.*");

	hFindFile = FindFirstFile(namebuf, pFileData);
	if (hFindFile == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		if (errno == ERROR_FILE_NOT_FOUND)
		    /*
		     *XXX for MSWIN32, why does os.listdir simply return an 
		     * empty list while the other versions raise OSError(ENOENT)?
		     *
		     * */
		    return PyErr_SetFromErrnoWithFilename(PyExc_OSError, namebuf);

		return win32_error("FindFirstFile", namebuf);
	}
	xldo->hFindFile = hFindFile;


    /* TODO: return NULL and set appropriate error if something goes wrong
     */

    return (PyObject*)xldo;
}

static PyObject *
xlistdir_closedir(PyXListdirObject *xldo)
{
    /* TODO: cleanup after the iteration
     *       clear any fields set in xldo 
     *       free any memory allocated
     *       
     *       NB: This function should not do anything terrible 
     *       if called multiple times.
     */
    HANDLE hFindFile = xldo->hFindFile;

    xldo->hFindFile = INVALID_HANDLE_VALUE; /*protect against multiple invocations*/

    if (hFindFile != INVALID_HANDLE_VALUE) {
	if (FindClose(xldo->hFindFile) == FALSE)
	    return win32_error("FindClose", xldo->namebuf);
    }
    return NULL;
}

static PyObject *
xlistdir_readdir(PyXListdirObject *xldo)
{
    PyObject *v;
    WIN32_FIND_DATA *pFileData = &(xldo->FileData);

    do
    {
	char* entry_name = pFileData->cFilename;
	
	if (entry_name[0] == '.' &&
	    (entry_name[1] == '\0' ||
	     (entry_name[1] == '.' &&
	     entry_name[2] == '\0')))
		continue;

	v = PyString_FromString(entry_name);
	if (v == NULL) {
	    break;
	}
	++xldo->itemno;
	return v;

    } while (FindNextFile(xldo->hFindFile, pFileData) == TRUE);

    /* clean up and return NULL, so we will raise StopIteration */
    return xlistdir_closedir(xldo);
}


#elif defined(_MSC_VER) 
/**** DOS/16-bit windows version ****/

typedef struct {
	PyObject_HEAD
	PyObject *dirname;
	int itemno;

	/* platform-specific fields */
	
#ifndef MAX_PATH
#define MAX_PATH	250
#endif
	char namebuf[MAX_PATH+5];
	struct _find_t ep;

} PyXListdirObject;

static PyObject *
xlistdir_opendir(PyXListdirObject *xldo)
{
   
    /* TODO: Set up xldo fields for the iteration.
     *       This will usually involve interpreting xldo->dirname
     *         and converting it to a format that the os can digest.
     */

	char *name, *pt;
	int len;
	PyObject *d, *v;
	char *namebuf = xldo->namebuf;
	struct _find_t *pep = xldo->ep;

	/*
	if (!PyArg_ParseTuple(args, "t#:listdir", &name, &len))
		return NULL;
	if (len >= MAX_PATH) {
		PyErr_SetString(PyExc_ValueError, "path too long");
		return NULL;
	}
	*/
	strcpy(namebuf, name);
	for (pt = namebuf; *pt; pt++)
		if (*pt == '/')
			*pt = '\\';
	if (namebuf[len-1] != '\\')
		namebuf[len++] = '\\';
	strcpy(namebuf + len, "*.*");


	if (_dos_findfirst(namebuf, _A_RDONLY |
			   _A_HIDDEN | _A_SYSTEM | _A_SUBDIR, &ep) != 0)
        {
		errno = ENOENT;
	        return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
	}

    /* TODO: return NULL and set appropriate error if something goes wrong
     */

    return (PyObject*)xldo;
}

static PyObject *
xlistdir_closedir(PyXListdirObject *xldo)
{
    /* TODO: cleanup after the iteration
     *       clear any fields set in xldo 
     *       free any memory allocated
     *       
     *       NB: This function should not do anything terrible 
     *       if called multiple times.
     */
    return NULL;
}

static PyObject *
xlistdir_readdir(PyXListdirObject *xldo)
{
	char *pt;
	PyObject *v;
	char *namebuf = op->namebuf;
	struct _find_t *pep = &(op->ep);

	do {
		char* entry_name = pep->name;

		if (entry_name[0] == '.' &&
		    (entry_name[1] == '\0' ||
		     entry_name[1] == '.' &&
		     entry_name[2] == '\0'))
			continue;
		
		strcpy(namebuf, entry_name);
		for (pt = namebuf; *pt; pt++)
			if (isupper(*pt))
				*pt = tolower(*pt);
		
		v = PyString_FromString(namebuf);
		if (v == NULL) {
			break;
		}
		++op->itemno;
		return v;
	} while (_dos_findnext(pep) == 0);


	/* clean up and return NULL, so we will raise StopIteration */
	return xlistdir_closedir(xldo);
}



#elif defined(PYOS_OS2)
/**** OS/2 version ****/

typedef struct {
	PyObject_HEAD
	PyObject *dirname;
	int itemno;

	/* platform-specific fields */
	
#ifndef MAX_PATH
#define MAX_PATH    CCHMAXPATH
#endif
	char namebuf[MAX_PATH+5];
	HDIR  hdir;
	FILEFINDBUF3   ep;
	ULONG srchcnt;


} PyXListdirObject;

static PyObject *
xlistdir_opendir(PyXListdirObject *xldo)
{
   
    /* TODO: Set up xldo fields for the iteration.
     *       This will usually involve interpreting xldo->dirname
     *         and converting it to a format that the os can digest.
     */

    /* TODO: return NULL and set appropriate error if something goes wrong
     */

    char *name, *pt;
    int len;
    PyObject *d, *v;
    char *namebuf = xldo->namebuf;
    FILEFINDBUF3   *pep = xldo->ep;
    APIRET rc;

    xldo->hdir = 1;
    xldo->srchcnt = 1;
    /*
    if (!PyArg_ParseTuple(args, "t#:listdir", &name, &len))
        return NULL;
    if (len >= MAX_PATH) {
		PyErr_SetString(PyExc_ValueError, "path too long");
        return NULL;
    }
    */
    strcpy(namebuf, name);
    for (pt = namebuf; *pt; pt++)
        if (*pt == '/')
            *pt = '\\';
    if (namebuf[len-1] != '\\')
        namebuf[len++] = '\\';
    strcpy(namebuf + len, "*.*");


    rc = DosFindFirst(namebuf,         /* Wildcard Pattern to Match */
                      &xldo->hdir,           /* Handle to Use While Search Directory */
                      FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                      pep, sizeof(xldo->ep), /* Structure to Receive Directory Entry */
                      &(xldo->srchcnt),     /* Max and Actual Count of Entries Per Iteration */
                      FIL_STANDARD);   /* Format of Entry (EAs or Not) */

    if (rc != NO_ERROR) {
        errno = ENOENT;
	return PyErr_SetFromErrnoWithFilename(PyExc_OSError,name);
    }
    return (PyObject*)xldo;
}

static PyObject *
xlistdir_closedir(PyXListdirObject *xldo)
{
    /* TODO: cleanup after the iteration
     *       clear any fields set in xldo 
     *       free any memory allocated
     *       
     *       NB: This function should not do anything terrible 
     *       if called multiple times.
     */
    return NULL;
}

static PyObject *
xlistdir_readdir(PyXListdirObject *xldo)
{
    PyObject *v;

    if (xldo->srchcnt > 0) { /* If Directory is NOT Totally Empty, */
	do {
	    char* entry_name = xldo->ep.achName;
	    char* namebuf = xldo->namebuf;

	    if (entry_name[0] == '.'
	    && (entry_name[1] == '\0' ||
		entry_name[1] == '.' && 
		entry_name[2] == '\0'))
		continue; /* Skip Over "." and ".." Names */

	    strcpy(namebuf, entry_name);

	    /* Leave Case of Name Alone -- In Native Form */
	    /* (Removed Forced Lowercasing Code) */

	    v = PyString_FromString(namebuf);
	    if (v == NULL) {
		break;
	    }
	    ++xldo->itemno;
	    return v;
	} while (DosFindNext(xldo->hdir, &(xldo->ep), sizeof(xldo->ep), 
		    &(xldo->srchcnt)) == NO_ERROR && xldo->srchcnt > 0);
    }

    /* clean up and return NULL, so we will raise StopIteration */
    return xlistdir_closedir(xldo);
}


#else
/**** posix version: for platforms with opendir/readdir/closedir ****/
typedef struct {
	PyObject_HEAD
	PyObject *dirname;
	int itemno;

	DIR *_dirp;
} PyXListdirObject;

static PyObject *
xlistdir_opendir(PyXListdirObject *xldo)
{
    DIR * dirp;
    char * dirname = PyString_AsString(xldo->dirname);
    
    xldo->_dirp = dirp = opendir(dirname);
    if (dirp == NULL ){
       return PyErr_SetFromErrnoWithFilename(PyExc_OSError, dirname);
    }
    return (PyObject*)xldo;
}

static PyObject *
xlistdir_closedir(PyXListdirObject *xldo)
{
    if(xldo->_dirp != NULL){
	int err = closedir(xldo->_dirp);
	xldo->_dirp = NULL;
	if(err){
	   return PyErr_SetFromErrno(PyExc_OSError);
	}
    }
    return NULL;
}

static PyObject *
xlistdir_readdir(PyXListdirObject *xldo)
{
    PyObject *v;
    struct dirent *ep;
    DIR * dirp = xldo->_dirp;


    while ((ep = readdir(dirp)) != NULL) 
    {
	if (ep->d_name[0] == '.' &&
	    (NAMLEN(ep) == 1 ||
	     (ep->d_name[1] == '.' && NAMLEN(ep) == 2)))
		continue;

	v = PyString_FromStringAndSize(ep->d_name, NAMLEN(ep));
	if (v == NULL) {
	    break;
	}
	++xldo->itemno;
	return v;
    }

    return xlistdir_closedir(xldo);
}
#endif /* posix version */


/**** the rest of this module should be platform-independent ****/


staticforward PyTypeObject XListdirObject_Type;

static void
xlistdir_dealloc(PyXListdirObject *op)
{
	Py_XDECREF(op->dirname);
	xlistdir_closedir(op);
	PyObject_DEL(op);
}

static PyXListdirObject *
newlistdirobject(PyObject *dirname)
{
	PyXListdirObject *op;
	op = PyObject_NEW(PyXListdirObject, &XListdirObject_Type);
	if (op == NULL)
		return NULL;
	Py_XINCREF(dirname);
	op->dirname = dirname;
	op->itemno = 0;
	/** XXX platform specific fields must be initialized in xlistdir_opendir
	 */
	return op;
}

static char xlistdir_doc [] =
"xlistdir(f)\n\
\n\
Return an xlistdir object for the dirname f.";

static PyObject *
xlistdir(PyObject *self, PyObject *args)
{
	PyObject *dirname;
	PyXListdirObject *ret;

	if (!PyArg_ParseTuple(args, "O:xlistdir", &dirname))
		return NULL;
	ret = newlistdirobject(dirname);
	return (PyObject*)xlistdir_opendir(ret);
}


static PyObject *
xlistdir_item(PyXListdirObject *a, int i)
{
	if (i != a->itemno) {
		PyErr_SetString(PyExc_RuntimeError,
			"xlistdir object accessed out of order");
		return NULL;
	}
	return xlistdir_readdir(a);
}

static PyObject *
xlistdir_getiter(PyXListdirObject *a)
{
	Py_INCREF(a);
	return (PyObject *)a;
}

static PyObject *
xlistdir_iternext(PyXListdirObject *a)
{
	PyObject *res;

	res = xlistdir_readdir(a);
	if (res == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
		PyErr_Clear();
	return res;
}

static PyObject *
xlistdir_next(PyXListdirObject *a, PyObject *args)
{
	PyObject *res;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	res = xlistdir_readdir(a);
	if (res == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
		PyErr_SetObject(PyExc_StopIteration, Py_None);
	return res;
}

static char next_doc[] = "x.next() -> the next line or raise StopIteration";

static PyMethodDef xlistdir_methods[] = {
	{"next", (PyCFunction)xlistdir_next, METH_VARARGS, next_doc},
	{NULL, NULL}
};

static PyObject *
xlistdir_getattr(PyObject *a, char *name)
{
	return Py_FindMethod(xlistdir_methods, a, name);
}

static PySequenceMethods xlistdir_as_sequence = {
	0, /*sq_length*/
	0, /*sq_concat*/
	0, /*sq_repeat*/
	(intargfunc)xlistdir_item, /*sq_item*/
};

static PyTypeObject XListdirObject_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"xlistdir.xlistdir",
	sizeof(PyXListdirObject),
	0,
	(destructor)xlistdir_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	xlistdir_getattr,			/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	&xlistdir_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)xlistdir_getiter,	/* tp_iter */
	(iternextfunc)xlistdir_iternext,	/* tp_iternext */
};

static PyMethodDef xlistdir_functions[] = {
	{"xlistdir", xlistdir, METH_VARARGS, xlistdir_doc},
	{NULL, NULL}
};

DL_EXPORT(void)
initxlistdir(void)
{
	XListdirObject_Type.ob_type = &PyType_Type;
	Py_InitModule("xlistdir", xlistdir_functions);
}
