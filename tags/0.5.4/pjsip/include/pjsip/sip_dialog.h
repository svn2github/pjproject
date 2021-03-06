/* $Id$ */
/* 
 * Copyright (C) 2003-2006 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJSIP_SIP_DIALOG_H__
#define __PJSIP_SIP_DIALOG_H__


/**
 * @file dialog.h
 * @brief SIP Dialog abstraction
 */

#include <pjsip/sip_msg.h>
#include <pjsip/sip_auth.h>
#include <pjsip/sip_errno.h>
#include <pj/sock.h>
#include <pj/assert.h>

PJ_BEGIN_DECL


/**
 * This structure is used to describe dialog's participants, which in this
 * case is local party (i.e. us) and remote party.
 */
typedef struct pjsip_dlg_party
{
    pjsip_fromto_hdr	*info;	    /**< From/To header, inc tag.	*/
    pj_str_t		 info_str;  /**< String rep of info header.	*/
    pj_uint32_t		 tag_hval;  /**< Hashed value of the tag.	*/
    pjsip_contact_hdr	*contact;   /**< Contact header.		*/
    pj_int32_t		 first_cseq;/**< First CSeq seen.		*/
    pj_int32_t		 cseq;	    /**< Next sequence number.		*/
} pjsip_dlg_party;


/**
 * Dialog state.
 */
enum pjsip_dialog_state
{
    PJSIP_DIALOG_STATE_NULL,
    PJSIP_DIALOG_STATE_ESTABLISHED,
};

/**
 * This structure describes the dialog structure. Application MUST NOT
 * try to SET the values here directly, but instead it MUST use the
 * appropriate dialog API. The dialog declaration only needs to be made 
 * visible because other PJSIP modules need to see it (e.g. INVITE session,
 * the event framework, etc.).
 *
 * Application MAY READ the dialog contents directly after it acquires
 * dialog lock.
 *
 * To acquire dialog lock, use #pjsip_dlg_inc_lock(), and to release it,
 * use #pjsip_dlg_dec_lock(). DO NOT USE pj_mutex_lock()/pj_mutex_unlock()
 * on the dialog's mutex directly, because this will not protect against
 * dialog being destroyed.
 */
struct pjsip_dialog
{
    /** The dialog set list. */
    PJ_DECL_LIST_MEMBER(pjsip_dialog);

    /* Dialog's system properties. */
    char		obj_name[PJ_MAX_OBJ_NAME];  /**< Standard id.	    */
    pj_pool_t	       *pool;	    /**< Dialog's pool.			    */
    pj_mutex_t	       *mutex;	    /**< Dialog's mutex. Do not call!!
					 Use pjsip_dlg_inc_lock() instead!  */
    pjsip_user_agent   *ua;	    /**< User agent instance.		    */
    pjsip_endpoint     *endpt;	    /**< Endpoint instance.		    */

    /* The dialog set. */
    void	       *dlg_set;

    /* Dialog's session properties. */
    pjsip_dialog_state	state;	    /**< Dialog state.			    */
    pjsip_uri	       *target;	    /**< Current target.		    */
    pjsip_hdr	        inv_hdr;    /**< Headers from hparam in dest URL    */
    pjsip_dlg_party     local;	    /**< Local party info.		    */
    pjsip_dlg_party     remote;	    /**< Remote party info.		    */
    pjsip_role_e	role;	    /**< Initial role.			    */
    pj_bool_t		secure;	    /**< Use secure transport?		    */
    pjsip_cid_hdr      *call_id;    /**< Call-ID header.		    */
    pjsip_route_hdr	route_set;  /**< Route set.			    */
    pjsip_auth_clt_sess	auth_sess;  /**< Client authentication session.	    */

    /** Session counter. */
    int			sess_count; /**< Number of sessions.		    */

    /** Transaction counter. */
    int			tsx_count;  /**< Number of pending transactions.    */

    /* Dialog usages. */
    unsigned		usage_cnt;  /**< Number of registered usages.	    */
    pjsip_module       *usage[PJSIP_MAX_MODULE]; /**< Array of usages, 
					 priority sorted		    */

    /** Module specific data. */
    void	       *mod_data[PJSIP_MAX_MODULE]; /**< Module data.	    */
};


/**
 * This utility function returns PJ_TRUE if the specified method is a
 * dialog creating request. This method property is used to determine
 * whether Contact header should be included in outgoing request.
 *
 * @param m		The SIP method.
 *
 * @return		PJ_TRUE if the method creates a dialog.
 */
PJ_DECL(pj_bool_t) pjsip_method_creates_dialog(const pjsip_method *m);

/**
 * Create a new dialog and return the instance in p_dlg parameter. 
 * After creating  the dialog, application can add modules as dialog usages
 * by calling  #pjsip_dlg_add_usage(). 
 *
 * If the request has To tag parameter, dialog's local tag will be initialized 
 * from this value. Otherwise a globally unique id generator will be invoked to
 * create dialog's local tag.
 *
 * This function also initializes the dialog's route set based on the 
 * Record-Route headers in the request, if present.
 *
 * Note that initially, the session count in the dialog will be initialized 
 * to zero.
 *
 * @param ua		    The user agent module instance.
 * @param local_uri	    Dialog local URI (i.e. From header).
 * @param local_contact_uri Optional dialog local Contact URI. 
 *			    If this argument is NULL, the Contact will be
 *			    taken from the local URI.
 * @param remote_uri	    Dialog remote URI (i.e. To header).
 * @param target	    Optional initial remote target. If this argument
 *			    is NULL, the initial target will be set to
 *			    remote URI.
 * @param p_dlg		    Pointer to receive the dialog.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_create_uac( pjsip_user_agent *ua,
					   const pj_str_t *local_uri,
					   const pj_str_t *local_contact_uri,
					   const pj_str_t *remote_uri,
					   const pj_str_t *target,
					   pjsip_dialog **p_dlg);


/**
 * Initialize UAS dialog from the information found in the incoming request 
 * that creates a dialog (such as INVITE, REFER, or SUBSCRIBE), and set the 
 * local Contact to contact. If contact is not specified, the local contact 
 * is initialized from the URI in the To header in the request. 
 *
 * This function will also create UAS transaction for the incoming request,
 * and associate the transaction to the rdata. Application can query the
 * transaction used to handle this request by calling #pjsip_rdata_get_tsx()
 * after this function returns.
 *
 * Note that initially, the session count in the dialog will be initialized 
 * to zero.
 *
 *
 * @param ua		    The user agent module instance.
 * @param rdata		    The incoming request that creates the dialog,
 *			    such as INVITE, SUBSCRIBE, or REFER.
 * @param contact	    Optional URI to be used as local Contact. If
 *			    this argument is NULL, the local contact will be
 *			    initialized from the value of To header in the
 *			    request.
 * @param p_dlg		    Pointer to receive the dialog.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_create_uas(  pjsip_user_agent *ua,
					    pjsip_rx_data *rdata,
					    const pj_str_t *contact,
					    pjsip_dialog **p_dlg);


/**
 * Create a new (forked) dialog on receipt on forked response in rdata. 
 * The new dialog will be created from original_dlg, except that it will have
 * new remote tag as copied from the To header in the response. Upon return, 
 * the new_dlg will have been registered to the user agent. Applications just 
 * need to add modules as dialog's usages.
 *
 * Note that initially, the session count in the dialog will be initialized 
 * to zero.
 *
 * @param original_dlg	    The original UAC dialog.
 * @param rdata		    The incoming forked response message.
 * @param new_dlg	    Pointer to receive the new dialog.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_fork(const pjsip_dialog *original_dlg,
				    const pjsip_rx_data *rdata,
				    pjsip_dialog **new_dlg );

/**
 * Forcefully terminate the dialog. Application can only call this function
 * when there is no session associated to the dialog. If there are sessions
 * that use this dialog, this function will refuse to terminate the dialog.
 * For this case, application MUST call the appropriate termination function 
 * for each dialog session (e.g. #pjsip_inv_terminate() to terminate INVITE
 * session).
 *
 * @param dlg		    The dialog.
 *
 * @return		    PJ_SUCCESS if dialog has been terminated.
 */
PJ_DECL(pj_status_t) pjsip_dlg_terminate( pjsip_dialog *dlg );


/**
 * Set dialog's initial route set to route_set list. This can only be called
 * for UAC dialog, before any request is sent. After dialog has been 
 * established, the route set can not be changed.
 *
 * For UAS dialog,the route set will be initialized in pjsip_dlg_create_uas()
 * from the Record-Route headers in the incoming request.
 *
 * The route_set argument is standard list of Route headers (i.e. with 
 * sentinel).
 *
 * @param dlg		    The UAC dialog.
 * @param route_set	    List of Route header.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_set_route_set( pjsip_dialog *dlg,
					      const pjsip_route_hdr *route_set );

/**
 * Increment the number of sessions in the dialog. Note that initially 
 * (after created) the dialog has the session counter set to zero.
 *
 * @param dlg		    The dialog.
 * @param mod		    The module that increments the session counter.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_inc_session( pjsip_dialog *dlg,
					    pjsip_module *mod);


/**
 * Decrement the number of sessions in the dialog. Once the session counter 
 * reach zero and there is no pending transaction, the dialog will be 
 * destroyed. Note that this function may destroy the dialog immediately 
 * if there is no pending transaction when this function is called.
 *
 * @param dlg		    The dialog.
 * @param mod		    The module that decrements the session counter.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_dec_session( pjsip_dialog *dlg,
					    pjsip_module *mod);

/**
 * Add a module as dialog usage, and optionally set the module specific data.
 *
 * @param dlg		    The dialog.
 * @param module	    The module to be registered as dialog usage.
 * @param mod_data	    Optional arbitrary data to be attached to dialog's
 *			    mod_data array at the module's index.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_add_usage( pjsip_dialog *dlg,
					  pjsip_module *module,
					  void *mod_data );

/**
 * Attach module specific data to the dialog. Application can also set 
 * the value directly by accessing dlg->mod_data[module_id].
 *
 * @param dlg		    The dialog
 * @param mod_id	    The ID of the module from which the data is to be
 *			    set to the dialog.
 * @param data		    Arbitrary data.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_set_mod_data( pjsip_dialog *dlg,
					     int mod_id,
					     void *data );

/**
 * Get module specific data previously attached to the dialog. Application
 * can also get value directly by accessing dlg->mod_data[module_id].
 *
 * @param dlg		    The dialog
 * @param mod_id	    The ID of the module from which the data is to be
 *			    retrieved from the dialog.
 *
 * @return		    The data that was previously set, or NULL.
 */
PJ_DECL(void*) pjsip_dlg_get_mod_data( pjsip_dialog *dlg,
				       int mod_id);


/**
 * Lock dialog and increment session counter termporarily, to prevent it 
 * from being destroyed.
 *
 * @param dlg		    The dialog.
 */
PJ_DECL(void) pjsip_dlg_inc_lock( pjsip_dialog *dlg );

/**
 * Unlock dialog and decrement temporary session counter. After this function
 * is called, dialog may be destroyed.
 *
 * @param dlg		    The dialog.
 */
PJ_DECL(void) pjsip_dlg_dec_lock( pjsip_dialog *dlg );


/**
 * Get the dialog instance in the incoming rdata. If an incoming message 
 * matches an existing dialog, the user agent must have put the matching 
 * dialog instance in the rdata, or otherwise this function will return 
 * NULL if the message didn't match any existing dialog.
 *
 * This function can only be called after endpoint distributes the message
 * to the transaction layer or UA layer. In other words, application can
 * only call this function in the context of module that runs in priority
 * number higher than PJSIP_MOD_PRIORITY_UA_PROXY_LAYER.
 *
 * @param rdata		    Incoming message buffer.
 *
 * @return		    The dialog instance that "owns" the message.
 */
PJ_DECL(pjsip_dialog*) pjsip_rdata_get_dlg( pjsip_rx_data *rdata );

/**
 * Get the associated dialog for the specified transaction, if any.
 *
 * @param tsx		    The transaction.
 *
 * @return		    The dialog instance which has been registered
 *			    to the transaction as transaction user, or
 *			    NULL if the transaction is outside any dialogs.
 */
PJ_DECL(pjsip_dialog*) pjsip_tsx_get_dlg( pjsip_transaction *tsx );


/**
 * Create a basic/generic request with the specified method and optionally
 * specify the cseq. Use value -1 for cseq to have the dialog automatically
 * put next cseq number for the request. Otherwise for some requests, 
 * e.q. CANCEL and ACK, application must put the CSeq in the original 
 * INVITE request as the parameter. 
 *
 * This function will also put Contact header where appropriate.
 *
 * @param dlg		    The dialog instance.
 * @param method	    The method of the request.
 * @param cseq		    Optional CSeq, which only needs to be specified
 *			    when creating ACK and CANCEL. For other requests,
 *			    specify -1 to use dialog's internal counter.
 * @param tdata		    Pointer to receive the request's transmit
 *			    data buffer.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_create_request(	pjsip_dialog *dlg,
						const pjsip_method *method,
						int cseq,
						pjsip_tx_data **tdata);


/**
 * Send request message to remote peer. If the request is not an ACK request, 
 * the dialog will send the request statefully, by creating an UAC transaction
 * and send the request with the transaction. 
 *
 * Also when the request is not ACK or CANCEL, the dialog will increment its
 * local cseq number and update the cseq in the request according to dialog's 
 * cseq.
 *
 * If p_tsx is not null, this argument will be set with the transaction 
 * instance that was used to send the request.
 *
 * This function will decrement the transmit data's reference counter
 * regardless the status of the operation.
 *
 * @param dlg		    The dialog.
 * @param tdata		    The request message to be sent.
 * @param p_tsx		    Optional argument to receive the transaction 
 *			    instance used to send the request.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_send_request (	pjsip_dialog *dlg,
						pjsip_tx_data *tdata,
						pjsip_transaction **p_tsx );


/**
 * Create a response message for the incoming request in rdata with status
 * code st_code and optional status text st_text. This function is different
 * than endpoint's API #pjsip_endpt_create_response() in that the dialog 
 * function adds Contact header and Record-Routes headers in the response 
 * where appropriate.
 *
 * @param dlg		    The dialog.
 * @param rdata		    The incoming request message for which the
 *			    response will be created.
 * @param st_code	    Status code.
 * @param st_text	    Optional string for custom status reason text.
 * @param tdata		    Pointer to receive the response message transmit
 *			    data buffer.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_create_response(	pjsip_dialog *dlg,
						pjsip_rx_data *rdata,
						int st_code,
						const pj_str_t *st_text,
						pjsip_tx_data **tdata);


/**
 * Modify previously sent response with other status code. Contact header 
 * will be added when appropriate.
 *
 * @param dlg		    The dialog.
 * @param tdata		    The transmit data buffer containing response
 *			    message to be modified.
 * @param st_code	    New status code to be set.
 * @param st_text	    Optional string for custom status reason text.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_modify_response(	pjsip_dialog *dlg,
						pjsip_tx_data *tdata,
						int st_code,
						const pj_str_t *st_text);


/**
 * Send response message statefully. The transaction instance MUST be the 
 * transaction that was reported on on_rx_request() callback.
 *
 * This function decrements the transmit data's reference counter regardless
 * the status of the operation.
 *
 * @param dlg		    The dialog.
 * @param tsx		    The UAS transaction associated with the incoming
 *			    request. If the request is within a dialog, or
 *			    a dialog has been created for the request that
 *			    creates the dialog, application can get the
 *			    transaction instance for the request by calling
 *			    #pjsip_rdata_get_tsx().
 * @param tdata		    Response message to be sent.
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_dlg_send_response(	pjsip_dialog *dlg,
						pjsip_transaction *tsx,
						pjsip_tx_data *tdata);


/**
 * This composite function sends response message statefully to an incoming
 * request message inside dialog.
 *
 * @param dlg	    The endpoint instance.
 * @param rdata	    The incoming request message.
 * @param st_code   Status code of the response.
 * @param st_text   Optional status text of the response.
 * @param hdr_list  Optional header list to be added to the response.
 * @param body	    Optional message body to be added to the response.
 *
 * @return	    PJ_SUCCESS if response message has successfully been
 *		    sent.
 */
PJ_DECL(pj_status_t) pjsip_dlg_respond( pjsip_dialog *dlg,
					pjsip_rx_data *rdata,
					int st_code,
					const pj_str_t *st_text,
					const pjsip_hdr *hdr_list,
					const pjsip_msg_body *body );


/**
 * @}
 */

/* 
 * Internal (called by sip_ua_layer.c)
 */

/* Receives transaction event (called by user_agent module) */
void pjsip_dlg_on_tsx_state( pjsip_dialog *dlg,
			     pjsip_transaction *tsx,
			     pjsip_event *e );

void pjsip_dlg_on_rx_request( pjsip_dialog *dlg,
			      pjsip_rx_data *rdata );

void pjsip_dlg_on_rx_response( pjsip_dialog *dlg,
			       pjsip_rx_data *rdata );



PJ_END_DECL


#endif	/* __PJSIP_SIP_DIALOG_H__ */

