#ifndef _MemMacro_H_
#define _MemMacro_H_
#pragma once

#define macCheckAndDel_Obj(a) if ((a) != NULL) { delete(a); (a) = NULL;}
#define macCheckAndDel_Array(a) if ((a) != NULL) { delete [](a); (a) = NULL;}


//#ifdef _DEBUG
//#include <crtdbg.h>
//#define new   new(_NORMAL_BLOCK, __FILE__,__LINE__)
//#endif

#endif
