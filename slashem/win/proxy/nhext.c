/* $Id$ */
/* Copyright (c) Slash'EM Development Team 2001 */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS
#include "hack.h"
#include "nhxdr.h"
#include "winproxy.h"

/*
 * This module implements the low-level NhExt protocols.
 */

static char request[1024];
static char reply[1024];
static int reply_len;
static NhExtXdr *in, *out;
static nhext_io_func read_f, write_f;
static void *read_h, *write_h;
static struct nhext_svc *callbacks;

int nhext_subprotocol1_init(nhext_io_func rf, void *rh, nhext_io_func wf, void *wh, struct nhext_svc *cb)
{
    in = (NhExtXdr *)alloc(sizeof(*out));
    out = (NhExtXdr *)alloc(sizeof(*out));
    nhext_xdrmem_create(in, reply, sizeof(reply), NHEXT_XDR_DECODE);
    nhext_xdrmem_create(out, request, sizeof(request), NHEXT_XDR_ENCODE);
    read_f = rf;
    read_h = rh;
    write_f = wf;
    write_h = wh;
    callbacks = cb;
    return 0;
}

void nhext_subprotocol1_end(void)
{
    nhext_xdr_destroy(in);
    nhext_xdr_destroy(out);
    free(in);
    free(out);
    in = NULL;
    out = NULL;
}

int nhext_rpc_vparams(NhExtXdr *xdrs, int no, va_list *app)
{
    int retval = TRUE;
    int param;
    va_list ap;
    long param_i;
    int *param_pi;
    char *param_s;
    char **param_ps, *param_pc;
    winid *param_pw;
    boolean param_b, *param_pb;
    int (*param_codec)();
    void *param_addr;
    ap = *app;
    while(retval && no--) {
	param = va_arg(ap, int);
	switch(param) {
	    case EXT_PARAM_INT:
		param_i = va_arg(ap, int);
		nhext_xdr_long(xdrs, &param_i);
		break;
	    case EXT_PARAM_STRING:
		param_s = va_arg(ap, char *);
		nhext_xdr_string(xdrs, &param_s, (unsigned int)-1);
		break;
	    case EXT_PARAM_WINID:
		param_i = va_arg(ap, winid);
		nhext_xdr_long(xdrs, &param_i);
		break;
	    case EXT_PARAM_BOOLEAN:
		param_b = va_arg(ap, boolean);
		nhext_xdr_bool(xdrs, &param_b);
		break;
	    case EXT_PARAM_CHAR:
		param_i = va_arg(ap, char);
		nhext_xdr_long(xdrs, &param_i);
		break;
	    case EXT_PARAM_PTR | EXT_PARAM_INT:
		param_pi = va_arg(ap, int *);
		retval = nhext_xdr_int(xdrs, param_pi);
		break;
	    case EXT_PARAM_PTR | EXT_PARAM_STRING:
		param_ps = va_arg(ap, char **);
		*param_ps = NULL;
		retval = nhext_xdr_string(xdrs, param_ps, (unsigned int)-1);
		break;
	    case EXT_PARAM_PTR | EXT_PARAM_WINID:
		param_pw = va_arg(ap, winid *);
		retval = nhext_xdr_long(xdrs, &param_i);
		*param_pw = (winid)param_i;
		break;
	    case EXT_PARAM_PTR | EXT_PARAM_BOOLEAN:
		param_pb = va_arg(ap, boolean *);
		retval = nhext_xdr_bool(xdrs, param_pb);
		break;
	    case EXT_PARAM_PTR | EXT_PARAM_CHAR:
		param_pc = va_arg(ap, char *);
		retval = nhext_xdr_char(xdrs, param_pc);
		break;
	    case EXT_PARAM_XDRF:
		param_codec = (int (*)())va_arg(ap, void *);
		param_addr = va_arg(ap, void *);
		retval = (*param_codec)(xdrs, param_addr);
		break;
	    default:
		impossible("Bad key in proxy rpc (%d)", param);
		retval = FALSE;
		break;
	}
    }
    *app = ap;
    return retval;
}

int nhext_rpc_params(NhExtXdr *xdrs, int no, ...)
{
    int retval;
    va_list ap;
    va_start(ap, no);
    retval = nhext_rpc_vparams(xdrs, no, &ap);
    va_end(ap);
    return retval;
}

/*
 * nhext_rpc() is a generic function to call a remote procedure and return
 * a result. It cannot deal with every possible type of request and result
 * packet, but it's very simple to use for those it can deal with.
 *
 * Arguments are:
 *	id		The ID of the procedure to call
 *	request		List of arguments describing the request packet
 *	response	List of arguments describing the response packet
 *
 * For both the request and response packets, the list of arguments consists
 * of:
 *
 *	no			The number of fields in the packet
 *	field[1] .. field[no]	The field to send/receive.
 */

int
nhext_rpc(unsigned short id, ...)
{
    va_list ap;
    unsigned long value;
    int i, retval;
    int len;		/* Length of the request packet */
    int no;		/* Number of fields */

    nhext_xdr_setpos(out, 4);		/* Leave space for value */
    va_start(ap, id);
    no = va_arg(ap, int);
    if (!nhext_rpc_vparams(out, no, &ap))
	return FALSE;
    len = nhext_xdr_getpos(out) - 4;
    value = (id << 16) | (len >> 2);
    nhext_xdr_setpos(out, 0);
    nhext_xdr_u_long(out, &value);
    if ((*write_f)(write_h, request, len + 4) != len + 4) {
	impossible("Write to proxy interface failed");
	return FALSE;
    }
    do
    {
	retval = nhext_svc(callbacks);
	if (retval < 0) {
	    impossible("Proxy server failed");
	    return FALSE;
	}
    } while(retval);
    id = (unsigned short)retval;
    no = va_arg(ap, int);
    if (!nhext_rpc_vparams(in, no, &ap))
	return FALSE;
    if (nhext_xdr_getpos(in) != reply_len) {
	impossible("Unexpected results in proxy rpc (%d, %d)", id, len);
	return FALSE;
    }
    return TRUE;
}

/*
 * nhext_svc() is a function to service incoming packets. It reads a packet
 * from the remote process (either a child or a parent). Service packets
 * (those with non-zero IDs) are dispatched and replied to, reply packets
 * are simply stored in the reply global buffer. The length of the packet
 * received will be stored in the reply_len variable.
 * In both cases, the ID is returned.
 *
 * nhext_svc() is thus suitable for use in two occasions. In the first, it
 * can be used to dispatch callbacks while waiting for a reply. In this mode
 * nhext_svc() should be called repeatedly until it returns 0. In the second
 * case, nhext_svc() can be used as the main loop of a server in which case
 * it should be called repeatedly to service requests and a zero ID should
 * be treated as an error.
 */

int
nhext_svc(struct nhext_svc *services)
{
    int i, len;
    unsigned short id;
    unsigned long value;
    reply_len = (*read_f)(read_h, reply, sizeof(reply));
    if (reply_len < 4) {
	if (reply_len > 0)  impossible("Bad reply packet (len %d)", reply_len);
	return -1;
    }
    nhext_xdr_setpos(in, 0);
    nhext_xdr_u_long(in, &value);
    id = value >> 16;
    if (reply_len - 4 != (value & 0xffff) << 2) {
	impossible("Bad reply packet (%d != %d)", reply_len - 4,
	  (value & 0xffff) << 2);
	return -1;
    }
    if (id)
    {
	nhext_xdr_setpos(out, 4);
	for(i = 0; services[i].id; i++) {
	    if (id == services[i].id) {
		(*services[i].handler)(id, in, out);
		break;
	    }
	}
	if (!services[i].id)
	    impossible("Unsupported proxy callback ID %d", id);
	len = nhext_xdr_getpos(out) - 4;
	if (len < 0 || len & 3)
	    impossible("Handler for proxy callback ID %d error", id);
	value = len >> 2;
	nhext_xdr_setpos(out, 0);
	nhext_xdr_u_long(out, &value);
	if ((*write_f)(write_h, request, len + 4) != len + 4) {
	    impossible("Write to proxy interface failed");
	    return -1;
	}
    }
    return id;
}