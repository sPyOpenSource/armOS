/* ssh.c
 *
 * Copyright (C) 2014-2016 wolfSSL Inc.
 *
 * This file is part of wolfSSH.
 *
 * wolfSSH is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSH is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfSSH.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * The ssh module contains the public API for wolfSSH.
 */


#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolf/ssh/headers/ssh.h>
#include <wolf/ssh/headers/internal.h>
#include <wolf/ssh/headers/log.h>
#include <wolf/crypt/headers/wc_port.h>
//#include "esp_log.h"

#ifdef NO_INLINE
    #include <wolf/ssh/misc.h>
#else
    #define WOLFSSH_MISC_INCLUDED
    #include "wolf/ssh/src/misc.c"
#endif


int wolfSSH_Init(void)
{
    int ret = WS_SUCCESS;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_Init()");
    if (wolfCrypt_Init() != 0)
        ret = WS_CRYPTO_FAILED;

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_Init(), returning %d", ret);
    return ret;
}


int wolfSSH_Cleanup(void)
{
    int ret = WS_SUCCESS;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_Cleanup()");

    if (wolfCrypt_Cleanup() != 0)
        ret = WS_CRYPTO_FAILED;

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_Cleanup(), returning %d", ret);
    return ret;
}


WOLFSSH_CTX* wolfSSH_CTX_new(byte side, void* heap)
{
    WOLFSSH_CTX* ctx;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_CTX_new()");

    if (side != WOLFSSH_ENDPOINT_SERVER && side != WOLFSSH_ENDPOINT_CLIENT) {
        WLOG(WS_LOG_DEBUG, "Invalid endpoint type");
        return NULL;
    }

    ctx = (WOLFSSH_CTX*)WMALLOC(sizeof(WOLFSSH_CTX), heap, DYNTYPE_CTX);
    ctx = CtxInit(ctx, side, heap);

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_CTX_new(), ctx = %p", ctx);

    return ctx;
}


void wolfSSH_CTX_free(WOLFSSH_CTX* ctx)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_CTX_free()");

    if (ctx) {
        CtxResourceFree(ctx);
        WFREE(ctx, ctx->heap, DYNTYPE_CTX);
    }
}


WOLFSSH* wolfSSH_new(WOLFSSH_CTX* ctx)
{
    WOLFSSH* ssh;
    void*    heap = NULL;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_new()");
    //ESP_LOGI("WOLFSSH", "Entering wolfSSH_new()");

    if (ctx)
        heap = ctx->heap;
    else {
        WLOG(WS_LOG_ERROR, "Trying to init a wolfSSH w/o wolfSSH_CTX");
        return NULL;
    }

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_new()");
    //ESP_LOGI("WOLFSSH", "ctx check ok, now malloc WOLFSSH");

    ssh = (WOLFSSH*)WMALLOC(sizeof(WOLFSSH), heap, DYNTYPE_SSH);
    //ESP_LOGI("WOLFSSH", "malloc done, SSH Initialization...");
    //if(ssh == NULL) ESP_LOGE("WOLFSSH", "ssh is NULL, passing to SshInit() anyway...");
    ssh = SshInit(ssh, ctx);
    //ESP_LOGI("WOLFSSH", "SshInit OK");
    //if(ssh == NULL) ESP_LOGE("WOLFSSH", "ssh is NULL after SshInit()");

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_new(), ssh = %p", ssh);
    //ESP_LOGI("WOLFSSH", "Leaving wolfSSH_new()");

    return ssh;
}


void wolfSSH_free(WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_free()");

    if (ssh) {
        void* heap = ssh->ctx ? ssh->ctx->heap : NULL;
        SshResourceFree(ssh, heap);
        WFREE(ssh, heap, DYNTYPE_SSH);
    }
}


int wolfSSH_set_fd(WOLFSSH* ssh, int fd)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_set_fd()");

    if (ssh) {
        ssh->rfd = fd;
        ssh->wfd = fd;

        ssh->ioReadCtx  = &ssh->rfd;
        ssh->ioWriteCtx = &ssh->wfd;

        return WS_SUCCESS;
    }
    return WS_BAD_ARGUMENT;
}


int wolfSSH_get_fd(const WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_get_fd()");

    if (ssh)
        return ssh->rfd;

    return WS_BAD_ARGUMENT;
}


int wolfSSH_SetHighwater(WOLFSSH* ssh, word32 highwater)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_SetHighwater()");

    if (ssh) {
        ssh->highwaterMark = highwater;

        return WS_SUCCESS;
    }

    return WS_BAD_ARGUMENT;
}


word32 wolfSSH_GetHighwater(WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_GetHighwater()");

    if (ssh)
        return ssh->highwaterMark;

    return 0;
}


void wolfSSH_SetHighwaterCb(WOLFSSH_CTX* ctx, word32 highwater,
                            WS_CallbackHighwater cb)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_SetHighwaterCb()");

    if (ctx) {
        ctx->highwaterMark = highwater;
        ctx->highwaterCb = cb;
    }
}


void wolfSSH_SetHighwaterCtx(WOLFSSH* ssh, void* ctx)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_SetHighwaterCtx()");

    if (ssh)
        ssh->highwaterCtx = ctx;
}


void* wolfSSH_GetHighwaterCtx(WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_GetHighwaterCtx()");

    if (ssh)
        return ssh->highwaterCtx;

    return NULL;
}


int wolfSSH_get_error(const WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_get_error()");

    if (ssh)
        return ssh->error;

    return WS_BAD_ARGUMENT;
}


const char* wolfSSH_get_error_name(const WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_get_error_name()");

    if (ssh)
        return GetErrorString(ssh->error);

    return NULL;
}


const char acceptError[] = "accept error: %s, %d";
const char acceptState[] = "accept state: %s";


int wolfSSH_accept(WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_accept()");
    //ESP_LOGI("WOLFSSH", "Entering wolfSSH_accept()");

    if (ssh == NULL)
        return WS_BAD_ARGUMENT;

    //ESP_LOGI("WOLFSSH", "ssh is not NULL");

    switch (ssh->acceptState) {

        case ACCEPT_BEGIN:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_BEGIN");
            if ( (ssh->error = SendProtoId(ssh)) < WS_SUCCESS) {
            	while(ssh->error == WS_WANT_READ) {
            		// non-blocking tcp
            		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
            		ssh->error = DoReceive(ssh);
            	}
            	if(ssh->error == WS_SUCCESS) break;
                WLOG(WS_LOG_DEBUG, acceptError, "BEGIN", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->acceptState = ACCEPT_SERVER_VERSION_SENT;
            WLOG(WS_LOG_DEBUG, acceptState, "SERVER_VERSION_SENT");
            FALL_THROUGH;

        case ACCEPT_SERVER_VERSION_SENT:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_SERVER_VERSION_SENT");
            while (ssh->clientState < CLIENT_VERSION_DONE) {
                if ( (ssh->error = DoProtoId(ssh)) < WS_SUCCESS) {
                	while(ssh->error == WS_WANT_READ) {
                		// non-blocking tcp
                		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
                		ssh->error = DoReceive(ssh);
                	}
                	if(ssh->error == WS_SUCCESS) break;
                    WLOG(WS_LOG_DEBUG, acceptError,
                         "SERVER_VERSION_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->acceptState = ACCEPT_CLIENT_VERSION_DONE;
            WLOG(WS_LOG_DEBUG, acceptState, "CLIENT_VERSION_DONE");
            FALL_THROUGH;

        case ACCEPT_CLIENT_VERSION_DONE:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_CLIENT_VERSION_DONE");
            if ( (ssh->error = SendKexInit(ssh)) < WS_SUCCESS) {
            	while(ssh->error == WS_WANT_READ) {
            		// non-blocking tcp
            		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
            		ssh->error = DoReceive(ssh);
            	}
            	if(ssh->error == WS_SUCCESS) break;
                WLOG(WS_LOG_DEBUG, acceptError,
                     "CLIENT_VERSION_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->acceptState = ACCEPT_SERVER_KEXINIT_SENT;
            WLOG(WS_LOG_DEBUG, acceptState, "SERVER_KEXINIT_SENT");
            FALL_THROUGH;

        case ACCEPT_SERVER_KEXINIT_SENT:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_SERVER_KEXINIT_SENT");
            while (ssh->isKeying) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                	while(ssh->error == WS_WANT_READ) {
                		// non-blocking tcp
                		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
                		ssh->error = DoReceive(ssh);
                	}
                	if(ssh->error == WS_SUCCESS) break;
                    WLOG(WS_LOG_DEBUG, acceptError,
                         "SERVER_KEXINIT_SENT", ssh->error);
                	//ESP_LOGE("WOLFSSH", "Accept error: SERVER_KEXINIT_SENT, %d", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->acceptState = ACCEPT_KEYED;
            WLOG(WS_LOG_DEBUG, acceptState, "KEYED");
            FALL_THROUGH;

        case ACCEPT_KEYED:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_KEYED");
            while (ssh->clientState < CLIENT_USERAUTH_REQUEST_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < 0) {
                	while(ssh->error == WS_WANT_READ) {
                		// non-blocking tcp
                		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
                		ssh->error = DoReceive(ssh);
                	}
                	if(ssh->error == WS_SUCCESS) break;
                    WLOG(WS_LOG_DEBUG, acceptError,
                         "KEYED", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->acceptState = ACCEPT_CLIENT_USERAUTH_REQUEST_DONE;
            WLOG(WS_LOG_DEBUG, acceptState, "CLIENT_USERAUTH_REQUEST_DONE");
            FALL_THROUGH;

        case ACCEPT_CLIENT_USERAUTH_REQUEST_DONE:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_CLIENT_USERAUTH_REQUEST_DONE");
            if ( (ssh->error = SendServiceAccept(ssh, ID_SERVICE_USERAUTH)) <
                                                                  WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, acceptError,
                     "CLIENT_USERAUTH_REQUEST_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->acceptState = ACCEPT_SERVER_USERAUTH_ACCEPT_SENT;
            WLOG(WS_LOG_DEBUG, acceptState,
                 "ACCEPT_SERVER_USERAUTH_ACCEPT_SENT");
            FALL_THROUGH;

        case ACCEPT_SERVER_USERAUTH_ACCEPT_SENT:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_SERVER_USERAUTH_ACCEPT_SENT");
            while (ssh->clientState < CLIENT_USERAUTH_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < 0) {
                	while(ssh->error == WS_WANT_READ) {
                		// non-blocking tcp
                		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
                		ssh->error = DoReceive(ssh);
                	}
                	if(ssh->error == WS_SUCCESS) break;
                    WLOG(WS_LOG_DEBUG, acceptError,
                         "SERVER_USERAUTH_ACCEPT_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->acceptState = ACCEPT_CLIENT_USERAUTH_DONE;
            WLOG(WS_LOG_DEBUG, acceptState, "CLIENT_USERAUTH_DONE");
            FALL_THROUGH;

        case ACCEPT_CLIENT_USERAUTH_DONE:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_CLIENT_USERAUTH_DONE");
            if ( (ssh->error = SendUserAuthSuccess(ssh)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, acceptError,
                     "CLIENT_USERAUTH_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->acceptState = ACCEPT_SERVER_USERAUTH_SENT;
            WLOG(WS_LOG_DEBUG, acceptState, "SERVER_USERAUTH_SENT");
            FALL_THROUGH;

        case ACCEPT_SERVER_USERAUTH_SENT:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_SERVER_USERAUTH_SENT");
            while (ssh->clientState < CLIENT_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < 0) {
                	while(ssh->error == WS_WANT_READ) {
                		// non-blocking tcp
                		//ESP_LOGW("WOLFSSH", "WANT_READ: looping");
                		ssh->error = DoReceive(ssh);
                	}
                	if(ssh->error == WS_SUCCESS) break;
                    WLOG(WS_LOG_DEBUG, acceptError,
                         "SERVER_USERAUTH_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->acceptState = ACCEPT_CLIENT_CHANNEL_REQUEST_DONE;
            WLOG(WS_LOG_DEBUG, acceptState, "CLIENT_CHANNEL_REQUEST_DONE");
            FALL_THROUGH;

        case ACCEPT_CLIENT_CHANNEL_REQUEST_DONE:
        	//ESP_LOGI("WOLFSSH", "acceptState = ACCEPT_CLIENT_CHANNEL_REQUEST_DONE");
            if ( (ssh->error = SendChannelOpenConf(ssh)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, acceptError,
                     "CLIENT_CHANNEL_REQUEST_DONE", ssh->error);
                //ESP_LOGE("WOLFSSH", "SendChannelOpenConf failed: %d", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->acceptState = ACCEPT_SERVER_CHANNEL_ACCEPT_SENT;
            WLOG(WS_LOG_DEBUG, acceptState, "SERVER_CHANNEL_ACCEPT_SENT");
    }

    return WS_SUCCESS;
}


const char connectError[] = "connect error: %s, %d";
const char connectState[] = "connect state: %s";


int wolfSSH_connect(WOLFSSH* ssh)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_connect()");

    if (ssh == NULL)
        return WS_BAD_ARGUMENT;

    switch (ssh->connectState) {

        case CONNECT_BEGIN:
            if ( (ssh->error = SendProtoId(ssh)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError, "BEGIN", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_VERSION_SENT;
            WLOG(WS_LOG_DEBUG, connectState, "CLIENT_VERSION_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_VERSION_SENT:
            while (ssh->serverState < SERVER_VERSION_DONE) {
                if ( (ssh->error = DoProtoId(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_VERSION_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_VERSION_DONE;
            WLOG(WS_LOG_DEBUG, connectState, "SERVER_VERSION_DONE");
            FALL_THROUGH;

        case CONNECT_SERVER_VERSION_DONE:
            if ( (ssh->error = SendKexInit(ssh)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, acceptError,
                     "SERVER_VERSION_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_KEXINIT_SENT;
            WLOG(WS_LOG_DEBUG, connectState, "CLIENT_KEXINIT_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_KEXINIT_SENT:
            while (ssh->serverState < SERVER_KEXINIT_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_KEXINIT_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_KEXINIT_DONE;
            WLOG(WS_LOG_DEBUG, connectState, "SERVER_KEXINIT_DONE");
            FALL_THROUGH;

        case CONNECT_SERVER_KEXINIT_DONE:
            if (ssh->handshake->kexId == ID_DH_GEX_SHA256)
                ssh->error = SendKexDhGexRequest(ssh);
            else
                ssh->error = SendKexDhInit(ssh);
            if (ssh->error < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError,
                     "SERVER_KEXINIT_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_KEXDH_INIT_SENT;
            WLOG(WS_LOG_DEBUG, connectState, "CLIENT_KEXDH_INIT_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_KEXDH_INIT_SENT:
            while (ssh->isKeying) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_KEXDH_INIT_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_KEYED;
            WLOG(WS_LOG_DEBUG, connectState, "KEYED");
            FALL_THROUGH;

        case CONNECT_KEYED:
            if ( (ssh->error = SendServiceRequest(ssh, ID_SERVICE_USERAUTH)) <
                                                                  WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError, "KEYED", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_USERAUTH_REQUEST_SENT;
            WLOG(WS_LOG_DEBUG, connectState, "CLIENT_USERAUTH_REQUEST_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_USERAUTH_REQUEST_SENT:
            while (ssh->serverState < SERVER_USERAUTH_REQUEST_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_USERAUTH_REQUEST_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_USERAUTH_REQUEST_DONE;
            WLOG(WS_LOG_DEBUG, connectState, "SERVER_USERAUTH_REQUEST_DONE");
            FALL_THROUGH;

        case CONNECT_SERVER_USERAUTH_REQUEST_DONE:
            if ( (ssh->error = SendUserAuthRequest(ssh, ID_NONE)) <
                                                                  WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError,
                     "SERVER_USERAUTH_REQUEST_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_USERAUTH_SENT;
            WLOG(WS_LOG_DEBUG, connectState, "CLIENT_USERAUTH_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_USERAUTH_SENT:
            while (ssh->serverState < SERVER_USERAUTH_ACCEPT_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_USERAUTH_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_USERAUTH_ACCEPT_DONE;
            WLOG(WS_LOG_DEBUG, connectState, "SERVER_USERAUTH_ACCEPT_DONE");
            FALL_THROUGH;

        case CONNECT_SERVER_USERAUTH_ACCEPT_DONE:
            if ( (ssh->error = SendChannelOpenSession(ssh, DEFAULT_WINDOW_SZ,
                                        DEFAULT_MAX_PACKET_SZ)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError,
                     "SERVER_USERAUTH_ACCEPT_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_CHANNEL_OPEN_SESSION_SENT;
            WLOG(WS_LOG_DEBUG, connectState,
                 "CLIENT_CHANNEL_OPEN_SESSION_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_CHANNEL_OPEN_SESSION_SENT:
            while (ssh->serverState < SERVER_CHANNEL_OPEN_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_CHANNEL_OPEN_SESSION_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_CHANNEL_OPEN_SESSION_DONE;
            WLOG(WS_LOG_DEBUG, connectState,
                 "SERVER_CHANNEL_OPEN_SESSION_DONE");
            FALL_THROUGH;

        case CONNECT_SERVER_CHANNEL_OPEN_SESSION_DONE:
            if ( (ssh->error = SendChannelRequestShell(ssh)) < WS_SUCCESS) {
                WLOG(WS_LOG_DEBUG, connectError,
                     "SERVER_CHANNEL_OPEN_SESSION_DONE", ssh->error);
                return WS_FATAL_ERROR;
            }
            ssh->connectState = CONNECT_CLIENT_CHANNEL_REQUEST_SHELL_SENT;
            WLOG(WS_LOG_DEBUG, connectState,
                 "CLIENT_CHANNEL_REQUEST_SHELL_SENT");
            FALL_THROUGH;

        case CONNECT_CLIENT_CHANNEL_REQUEST_SHELL_SENT:
            while (ssh->serverState < SERVER_DONE) {
                if ( (ssh->error = DoReceive(ssh)) < WS_SUCCESS) {
                    WLOG(WS_LOG_DEBUG, connectError,
                         "CLIENT_CHANNEL_REQUEST_SHELL_SENT", ssh->error);
                    return WS_FATAL_ERROR;
                }
            }
            ssh->connectState = CONNECT_SERVER_CHANNEL_REQUEST_SHELL_DONE;
            WLOG(WS_LOG_DEBUG, connectState,
                 "SERVER_CHANNEL_REQUEST_SHELL_DONE");
    }

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_connect()");
    return WS_SUCCESS;
}


int wolfSSH_shutdown(WOLFSSH* ssh)
{
    int ret = WS_SUCCESS;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_shutdown()");

    if (ssh == NULL)
        ret = WS_BAD_ARGUMENT;

    if (ret == WS_SUCCESS)
        ret = SendChannelEof(ssh, 0);

    if (ret == WS_SUCCESS)
        ret = SendChannelClose(ssh, 0);

    if (ret == WS_SUCCESS)
        ret = SendDisconnect(ssh, WOLFSSH_DISCONNECT_BY_APPLICATION);

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_shutdown(), ret = %d", ret);
    return ret;
}


int wolfSSH_TriggerKeyExchange(WOLFSSH* ssh)
{
    int ret = WS_SUCCESS;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_TriggerKeyExchange()");
    if (ssh == NULL)
        ret = WS_BAD_ARGUMENT;

    if (ret == WS_SUCCESS)
        ret = SendKexInit(ssh);

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_TriggerKeyExchange(), ret = %d", ret);
    return ret;
}


int wolfSSH_stream_read(WOLFSSH* ssh, byte* buf, word32 bufSz)
{
    Buffer* inputBuffer;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_stream_read()");
    //ESP_LOGI("WOLFSSH", "wolfSSH_stream_read()");

    if (ssh == NULL || buf == NULL || bufSz == 0 || ssh->channelList == NULL)
        return WS_BAD_ARGUMENT;

    inputBuffer = &ssh->channelList->inputBuffer;

    while (inputBuffer->length - inputBuffer->idx == 0) {
        int ret = DoReceive(ssh);
        if (ret < 0) {
            WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_stream_read(), ret = %d", ret);
            return ret;
        }
    }
    //ESP_LOGI("WOLFSSH", "wolfSSH_stream_read: while loop done");

    bufSz = min(bufSz, inputBuffer->length - inputBuffer->idx);
    WMEMCPY(buf, inputBuffer->buffer + inputBuffer->idx, bufSz);
    inputBuffer->idx += bufSz;

    if (!ssh->isKeying && (inputBuffer->length > inputBuffer->bufferSz / 2)) {

        word32 usedSz = inputBuffer->length - inputBuffer->idx;
        word32 bytesToAdd = inputBuffer->idx;
        int sendResult;

        WLOG(WS_LOG_DEBUG, "Making more room: %u", usedSz);
        if (usedSz) {
            WLOG(WS_LOG_DEBUG, "  ...moving data down");
            WMEMMOVE(inputBuffer->buffer,
                     inputBuffer->buffer + bytesToAdd, usedSz);
        }

        sendResult = SendChannelWindowAdjust(ssh,
                                             ssh->channelList->peerChannel,
                                             bytesToAdd);
        if (sendResult != WS_SUCCESS)
            bufSz = sendResult;

        WLOG(WS_LOG_INFO, "  bytesToAdd = %u", bytesToAdd);
        WLOG(WS_LOG_INFO, "  windowSz = %u", ssh->channelList->windowSz);
        ssh->channelList->windowSz += bytesToAdd;
        WLOG(WS_LOG_INFO, "  update windowSz = %u", ssh->channelList->windowSz);

        inputBuffer->length = usedSz;
        inputBuffer->idx = 0;
    }

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_stream_read(), rxd = %d", bufSz);
    return bufSz;
}


int wolfSSH_stream_send(WOLFSSH* ssh, byte* buf, word32 bufSz)
{
    int bytesTxd = 0;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_stream_send()");

    if (ssh == NULL || buf == NULL || ssh->channelList == NULL)
        return WS_BAD_ARGUMENT;

    bytesTxd = SendChannelData(ssh, ssh->channelList->peerChannel, buf, bufSz);

    WLOG(WS_LOG_DEBUG, "Leaving wolfSSH_stream_send(), txd = %d", bytesTxd);
    return bytesTxd;
}


void wolfSSH_SetUserAuth(WOLFSSH_CTX* ctx, WS_CallbackUserAuth cb)
{
    if (ctx != NULL) {
        ctx->userAuthCb = cb;
    }
}


void wolfSSH_SetUserAuthCtx(WOLFSSH* ssh, void* userAuthCtx)
{
    if (ssh != NULL) {
        ssh->userAuthCtx = userAuthCtx;
    }
}


void* wolfSSH_GetUserAuthCtx(WOLFSSH* ssh)
{
    if (ssh != NULL) {
        return ssh->userAuthCtx;
    }
    return NULL;
}


int wolfSSH_SetUsername(WOLFSSH* ssh, const char* username)
{
    char* value = NULL;
    word32 valueSz;
    int ret = WS_SUCCESS;

    if (ssh == NULL || ssh->handshake == NULL ||
        ssh->ctx->side == WOLFSSH_ENDPOINT_SERVER ||
        username == NULL) {

        ret = WS_BAD_ARGUMENT;
    }

    if (ret == WS_SUCCESS) {
        valueSz = (word32)WSTRLEN(username);
        if (valueSz > 0)
            value = (char*)WMALLOC(valueSz + 1, ssh->ctx->heap, DYNTYPE_STRING);
        if (value == NULL)
            ret = WS_MEMORY_E;
    }

    if (ret == WS_SUCCESS) {
        WSTRNCPY(value, username, valueSz + 1);
        if (ssh->userName != NULL) {
            WFREE(ssh->userName, heap, DYNTYPE_STRING);
            ssh->userName = NULL;
        }
        ssh->userName = value;
        ssh->userNameSz = valueSz;
    }

    return ret;
}


int wolfSSH_CTX_SetBanner(WOLFSSH_CTX* ctx,
                          const char* newBanner)
{
    word32 newBannerSz = 0;

    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_CTX_SetBanner()");

    if (ctx == NULL)
        return WS_BAD_ARGUMENT;

    if (newBanner != NULL) {
        WLOG(WS_LOG_INFO, "  setting banner to: \"%s\"", newBanner);
        newBannerSz = (word32)WSTRLEN(newBanner);
    }

    ctx->banner = newBanner;
    ctx->bannerSz = newBannerSz;

    return WS_SUCCESS;
}


int wolfSSH_CTX_UsePrivateKey_buffer(WOLFSSH_CTX* ctx,
                                   const byte* in, word32 inSz, int format)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_CTX_UsePrivateKey_buffer()");
    return wolfSSH_ProcessBuffer(ctx, in, inSz, format, BUFTYPE_PRIVKEY); 
}


void wolfSSH_GetStats(WOLFSSH* ssh, word32* txCount, word32* rxCount,
                      word32* seq, word32* peerSeq)
{
    word32 rTxCount = 0;
    word32 rRxCount = 0;
    word32 rSeq = 0;
    word32 rPeerSeq = 0;

    if (ssh != NULL) {
        rTxCount = ssh->txCount;
        rRxCount = ssh->rxCount;
        rSeq = ssh->seq;
        rPeerSeq = ssh->peerSeq;
    }

    if (txCount != NULL)
        *txCount = rTxCount;
    if (rxCount != NULL)
        *rxCount = rRxCount;
    if (seq != NULL)
        *seq = rSeq;
    if (peerSeq != NULL)
        *peerSeq = rPeerSeq;
}


int wolfSSH_KDF(byte hashId, byte keyId,
                byte* key, word32 keySz,
                const byte* k, word32 kSz,
                const byte* h, word32 hSz,
                const byte* sessionId, word32 sessionIdSz)
{
    WLOG(WS_LOG_DEBUG, "Entering wolfSSH_KDF()");
    return GenerateKey(hashId, keyId, key, keySz, k, kSz, h, hSz,
                       sessionId, sessionIdSz);
}
