dnl $Id$
dnl config.m4 for extension zts_research

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(zts_research, for zts_research support,
dnl Make sure that the comment is aligned:
dnl [  --with-zts_research             Include zts_research support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(zts_research, whether to enable zts_research support,
dnl Make sure that the comment is aligned:
dnl [  --enable-zts_research           Enable zts_research support])

if test "$PHP_ZTS_RESEARCH" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-zts_research -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/zts_research.h"  # you most likely want to change this
  dnl if test -r $PHP_ZTS_RESEARCH/$SEARCH_FOR; then # path given as parameter
  dnl   ZTS_RESEARCH_DIR=$PHP_ZTS_RESEARCH
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for zts_research files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       ZTS_RESEARCH_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$ZTS_RESEARCH_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the zts_research distribution])
  dnl fi

  dnl # --with-zts_research -> add include path
  dnl PHP_ADD_INCLUDE($ZTS_RESEARCH_DIR/include)

  dnl # --with-zts_research -> check for lib and symbol presence
  dnl LIBNAME=zts_research # you may want to change this
  dnl LIBSYMBOL=zts_research # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $ZTS_RESEARCH_DIR/lib, ZTS_RESEARCH_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_ZTS_RESEARCHLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong zts_research lib version or lib not found])
  dnl ],[
  dnl   -L$ZTS_RESEARCH_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(ZTS_RESEARCH_SHARED_LIBADD)

  PHP_NEW_EXTENSION(zts_research, zts_research.c, $ext_shared)
fi
