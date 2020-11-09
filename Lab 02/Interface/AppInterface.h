

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for AppInterface.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, app_config, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __AppInterface_h__
#define __AppInterface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __Interface_INTERFACE_DEFINED__
#define __Interface_INTERFACE_DEFINED__

/* interface Interface */
/* [implicit_handle][version][uuid] */ 

int serv_login( 
    /* [string][in] */ const unsigned char *login,
    /* [string][in] */ const unsigned char *password);

int serv_remove( 
    /* [string][in] */ const unsigned char *path_file);

int serv_download( 
    /* [string][in] */ const unsigned char *path_from,
    /* [size_is][out] */ unsigned char *buffer,
    /* [in] */ int buffer_size);

int serv_upload( 
    /* [string][in] */ const unsigned char *path_to,
    /* [size_is][in] */ const unsigned char *buffer,
    /* [in] */ int buffer_size);

int serv_get_file_size( 
    /* [string][in] */ const unsigned char *path_file);


extern handle_t InterfaceHandle;


extern RPC_IF_HANDLE Interface_v1_0_c_ifspec;
extern RPC_IF_HANDLE Interface_v1_0_s_ifspec;
#endif /* __Interface_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


