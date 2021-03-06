/* $Id$ */
/* 
 * Copyright (C) 2003-2005 Benny Prijono <benny@prijono.org>
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
#ifndef __PJNATH_ICE_SESSION_H__
#define __PJNATH_ICE_SESSION_H__

/**
 * @file ice_session.h
 * @brief ICE session management
 */
#include <pjnath/types.h>
#include <pjnath/stun_session.h>
#include <pj/sock.h>
#include <pj/timer.h>

/**
 * @defgroup PJNATH_ICE Interactive Connectivity Establishment (ICE)
 * @brief Interactive Connectivity Establishment (ICE)
 */


PJ_BEGIN_DECL


/**
 * @defgroup PJNATH_ICE_SESSION ICE Session
 * @brief Transport Independent ICE Session
 * @ingroup PJNATH_ICE
 * @{
 *
 * This module describes #pj_ice_sess, a transport independent ICE session,
 * part of PJNATH - the Open Source NAT helper library.
 *
 * An ICE session, represented by #pj_ice_sess structure, is the lowest 
 * abstraction of ICE in PJNATH, and it is used to perform and manage
 * connectivity checks of transport address candidates <b>within a
 * single media stream</b> (note: this differs from what is described
 * in ICE draft, where an ICE session manages the whole media sessions
 * rather than just a single stream).
 *
 * The ICE session described here is independent from any transports,
 * meaning that the actual network I/O for this session would have to
 * be performed by the application, or higher layer abstraction. 
 * Using this framework, application would give any incoming packets to
 * the ICE session, and it would provide the ICE session with a callback
 * to send outgoing message.
 *
 * For higher abstraction of ICE where transport is included, please 
 * see \ref PJNATH_ICE_STREAM_TRANSPORT.
 */

/**
 * This enumeration describes the type of an ICE candidate.
 */
typedef enum pj_ice_cand_type
{
    /**
     * ICE host candidate. A host candidate represents the actual local
     * transport address in the host.
     */
    PJ_ICE_CAND_TYPE_HOST,

    /**
     * ICE server reflexive candidate, which represents the public mapped
     * address of the local address, and is obtained by sending STUN
     * Binding request from the host candidate to a STUN server.
     */
    PJ_ICE_CAND_TYPE_SRFLX,

    /**
     * ICE peer reflexive candidate, which is the address as seen by peer
     * agent during connectivity check.
     */
    PJ_ICE_CAND_TYPE_PRFLX,

    /**
     * ICE relayed candidate, which represents the address allocated in
     * TURN server.
     */
    PJ_ICE_CAND_TYPE_RELAYED

} pj_ice_cand_type;


/** Forward declaration for pj_ice_sess */
typedef struct pj_ice_sess pj_ice_sess;

/** Forward declaration for pj_ice_sess_check */
typedef struct pj_ice_sess_check pj_ice_sess_check;


/**
 * This structure describes ICE component. 
 * A media stream may require multiple components, each of which has 
 * to work for the media stream as a whole to work.  For media streams
 * based on RTP, there are two components per media stream - one for RTP,
 * and one for RTCP.
 */
typedef struct pj_ice_sess_comp
{
    /**
     * The pointer to ICE check which was nominated for this component.
     * The value will be NULL if a nominated check has not been found
     * for this component.
     */
    pj_ice_sess_check	*valid_check;

    /**
     * The STUN session to be used to send and receive STUN messages for this
     * component.
     */
    pj_stun_session	*stun_sess;

} pj_ice_sess_comp;


/**
 * This structure describes an ICE candidate.
 * ICE candidate is a transport address that is to be tested by ICE
 * procedures in order to determine its suitability for usage for
 * receipt of media.  Candidates also have properties - their type
 * (server reflexive, relayed or host), priority, foundation, and
 * base.
 */
typedef struct pj_ice_sess_cand
{
    /**
     * The component ID of this candidate. Note that component IDs starts
     * with one for RTP and two for RTCP. In other words, it's not zero
     * based.
     */
    pj_uint32_t		 comp_id;

    /**
     * The candidate type, as described in #pj_ice_cand_type enumeration.
     */
    pj_ice_cand_type	 type;

    /**
     * The foundation string, which is an identifier which value will be
     * equivalent for two candidates that are of the same type, share the 
     * same base, and come from the same STUN server. The foundation is 
     * used to optimize ICE performance in the Frozen algorithm.
     */
    pj_str_t		 foundation;

    /**
     * The candidate's priority, a 32-bit unsigned value which value will be
     * calculated by the ICE session when a candidate is registered to the
     * ICE session.
     */
    pj_uint32_t		 prio;

    /**
     * IP address of this candidate. For host candidates, this represents
     * the local address of the socket. For reflexive candidates, the value
     * will be the public address allocated in NAT router for the host
     * candidate and as reported in MAPPED-ADDRESS or XOR-MAPPED-ADDRESS
     * attribute of STUN Binding request. For relayed candidate, the value 
     * will be the address allocated in the TURN server by STUN Allocate
     * request.
     */
    pj_sockaddr		 addr;

    /**
     * Base address of this candidate. "Base" refers to the address an agent 
     * sends from for a particular candidate.  For host candidates, the base
     * is the same as the host candidate itself. For reflexive candidates, 
     * the base is the local IP address of the socket. For relayed candidates,
     * the base address is the transport address allocated in the TURN server
     * for this candidate.
     */
    pj_sockaddr		 base_addr;

    /**
     * Related address, which is used for informational only and is not used
     * in any way by the ICE session.
     */
    pj_sockaddr		 rel_addr;

} pj_ice_sess_cand;


/**
 * This enumeration describes the state of ICE check.
 */
typedef enum pj_ice_sess_check_state
{
    /**
     * A check for this pair hasn't been performed, and it can't
     * yet be performed until some other check succeeds, allowing this
     * pair to unfreeze and move into the Waiting state.
     */
    PJ_ICE_SESS_CHECK_STATE_FROZEN,

    /**
     * A check has not been performed for this pair, and can be
     * performed as soon as it is the highest priority Waiting pair on
     * the check list.
     */
    PJ_ICE_SESS_CHECK_STATE_WAITING,

    /**
     * A check has not been performed for this pair, and can be
     * performed as soon as it is the highest priority Waiting pair on
     * the check list.
     */
    PJ_ICE_SESS_CHECK_STATE_IN_PROGRESS,

    /**
     * A check has not been performed for this pair, and can be
     * performed as soon as it is the highest priority Waiting pair on
     * the check list.
     */
    PJ_ICE_SESS_CHECK_STATE_SUCCEEDED,

    /**
     * A check for this pair was already done and failed, either
     * never producing any response or producing an unrecoverable failure
     * response.
     */
    PJ_ICE_SESS_CHECK_STATE_FAILED

} pj_ice_sess_check_state;


/**
 * This structure describes an ICE connectivity check. An ICE check
 * contains a candidate pair, and will involve sending STUN Binding 
 * Request transaction for the purposes of verifying connectivity. 
 * A check is sent from the local candidate to the remote candidate 
 * of a candidate pair.
 */
struct pj_ice_sess_check
{
    /**
     * Pointer to local candidate entry of this check.
     */
    pj_ice_sess_cand	*lcand;

    /**
     * Pointer to remote candidate entry of this check.
     */
    pj_ice_sess_cand	*rcand;

    /**
     * Check priority.
     */
    pj_timestamp	 prio;

    /**
     * Connectivity check state.
     */
    pj_ice_sess_check_state	 state;

    /**
     * STUN transmit data containing STUN Binding request that was sent 
     * as part of this check. The value will only be set when this check 
     * has a pending transaction, and is used to cancel the transaction
     * when other check has succeeded.
     */
    pj_stun_tx_data	*tdata;

    /**
     * Flag to indicate whether this check is nominated. A nominated check
     * contains USE-CANDIDATE attribute in its STUN Binding request.
     */
    pj_bool_t		 nominated;

    /**
     * When the check failed, this will contain the failure status of the
     * STUN transaction.
     */
    pj_status_t		 err_code;
};


/**
 * This enumeration describes ICE checklist state.
 */
typedef enum pj_ice_sess_checklist_state
{
    /**
     * The checklist is not yet running.
     */
    PJ_ICE_SESS_CHECKLIST_ST_IDLE,

    /**
     * In this state, ICE checks are still in progress for this
     * media stream.
     */
    PJ_ICE_SESS_CHECKLIST_ST_RUNNING,

    /**
     * In this state, ICE checks have completed for this media stream,
     * either successfully or with failure.
     */
    PJ_ICE_SESS_CHECKLIST_ST_COMPLETED

} pj_ice_sess_checklist_state;


/**
 * This structure represents ICE check list, that is an ordered set of 
 * candidate pairs that an agent will use to generate checks.
 */
typedef struct pj_ice_sess_checklist
{
    /**
     * The checklist state.
     */
    pj_ice_sess_checklist_state   state;

    /**
     * Number of candidate pairs (checks).
     */
    unsigned		     count;

    /**
     * Array of candidate pairs (checks).
     */
    pj_ice_sess_check	     checks[PJ_ICE_MAX_CHECKS];

    /**
     * A timer used to perform periodic check for this checklist.
     */
    pj_timer_entry	     timer;

} pj_ice_sess_checklist;


/**
 * This structure contains callbacks that will be called by the ICE
 * session.
 */
typedef struct pj_ice_sess_cb
{
    /**
     * An optional callback that will be called by the ICE session when
     * ICE negotiation has completed, successfully or with failure.
     *
     * @param ice	    The ICE session.
     * @param status	    Will contain PJ_SUCCESS if ICE negotiation is
     *			    successful, or some error code.
     */
    void	(*on_ice_complete)(pj_ice_sess *ice, pj_status_t status);

    /**
     * A mandatory callback which will be called by the ICE session when
     * it needs to send outgoing STUN packet. 
     *
     * @param ice	    The ICE session.
     * @param comp_id	    ICE component ID.
     * @param pkt	    The STUN packet.
     * @param size	    The size of the packet.
     * @param dst_addr	    Packet destination address.
     * @param dst_addr_len  Length of destination address.
     */
    pj_status_t (*on_tx_pkt)(pj_ice_sess *ice, unsigned comp_id, 
			     const void *pkt, pj_size_t size,
			     const pj_sockaddr_t *dst_addr,
			     unsigned dst_addr_len);

    /**
     * A mandatory callback which will be called by the ICE session when
     * it receives packet which is not part of ICE negotiation.
     *
     * @param ice	    The ICE session.
     * @param comp_id	    ICE component ID.
     * @param pkt	    The whole packet.
     * @param size	    Size of the packet.
     * @param src_addr	    Source address where this packet was received 
     *			    from.
     * @param src_addr_len  The length of source address.
     */
    void	(*on_rx_data)(pj_ice_sess *ice, unsigned comp_id,
			      void *pkt, pj_size_t size,
			      const pj_sockaddr_t *src_addr,
			      unsigned src_addr_len);
} pj_ice_sess_cb;


/**
 * This enumeration describes the role of the ICE agent.
 */
typedef enum pj_ice_sess_role
{
    /**
     * The ICE agent is in controlled role.
     */
    PJ_ICE_SESS_ROLE_UNKNOWN,

    /**
     * The ICE agent is in controlled role.
     */
    PJ_ICE_SESS_ROLE_CONTROLLED,

    /**
     * The ICE agent is in controlling role.
     */
    PJ_ICE_SESS_ROLE_CONTROLLING

} pj_ice_sess_role;


/**
 * This structure represents an incoming check (an incoming Binding
 * request message), and is mainly used to keep early checks in the
 * list in the ICE session. An early check is a request received
 * from remote when we haven't received SDP answer yet, therefore we
 * can't perform triggered check. For such cases, keep the incoming
 * request in a list, and we'll do triggered checks (simultaneously)
 * as soon as we receive answer.
 */
typedef struct pj_ice_rx_check
{
    PJ_DECL_LIST_MEMBER(struct pj_ice_rx_check);

    unsigned		 comp_id;	/**< Component ID.		*/

    pj_sockaddr		 src_addr;	/**< Source address of request	*/
    unsigned		 src_addr_len;	/**< Length of src address.	*/

    pj_bool_t		 use_candidate;	/**< USE-CANDIDATE is present?	*/
    pj_uint32_t		 priority;	/**< PRIORITY value in the req.	*/
    pj_stun_uint64_attr *role_attr;	/**< ICE-CONTROLLING/CONTROLLED	*/

} pj_ice_rx_check;


/**
 * This structure describes the ICE session. For this version of PJNATH,
 * an ICE session corresponds to a single media stream (unlike the ICE
 * session described in the ICE standard where an ICE session covers the
 * whole media and may consist of multiple media streams). The decision
 * to support only a single media session was chosen for simplicity,
 * while still allowing application to utilize multiple media streams by
 * creating multiple ICE sessions, one for each media stream.
 */
struct pj_ice_sess
{
    char		obj_name[PJ_MAX_OBJ_NAME];  /**< Object name.	    */

    pj_pool_t		*pool;			    /**< Pool instance.	    */
    void		*user_data;		    /**< App. data.	    */
    pj_mutex_t		*mutex;			    /**< Mutex.		    */
    pj_ice_sess_role	 role;			    /**< ICE role.	    */
    pj_timestamp	 tie_breaker;		    /**< Tie breaker value  */
    pj_uint8_t		*prefs;			    /**< Type preference.   */
    pj_bool_t		 is_complete;		    /**< Complete?	    */
    pj_status_t		 ice_status;		    /**< Error status.	    */
    pj_ice_sess_cb	 cb;			    /**< Callback.	    */

    pj_stun_config	 stun_cfg;		    /**< STUN settings.	    */

    /* STUN credentials */
    pj_str_t		 tx_ufrag;		    /**< Remote ufrag.	    */
    pj_str_t		 tx_uname;		    /**< Uname for TX.	    */
    pj_str_t		 tx_pass;		    /**< Remote password.   */
    pj_str_t		 rx_ufrag;		    /**< Local ufrag.	    */
    pj_str_t		 rx_uname;		    /**< Uname for RX	    */
    pj_str_t		 rx_pass;		    /**< Local password.    */

    /* Components */
    unsigned		 comp_cnt;		    /**< # of components.   */
    pj_ice_sess_comp	 comp[PJ_ICE_MAX_COMP];	    /**< Component array    */

    /* Local candidates */
    unsigned		 lcand_cnt;		    /**< # of local cand.   */
    pj_ice_sess_cand	 lcand[PJ_ICE_MAX_CAND];    /**< Array of cand.	    */

    /* Remote candidates */
    unsigned		 rcand_cnt;		    /**< # of remote cand.  */
    pj_ice_sess_cand	 rcand[PJ_ICE_MAX_CAND];    /**< Array of cand.	    */

    /* List of eearly checks */
    pj_ice_rx_check	 early_check;		    /**< Early checks.	    */

    /* Checklist */
    pj_ice_sess_checklist clist;		    /**< Active checklist   */
    
    /* Valid list */
    pj_ice_sess_checklist valid_list;		    /**< Valid list.	    */
};


/**
 * This is a utility function to retrieve the string name for the
 * particular candidate type.
 *
 * @param type		Candidate type.
 *
 * @return		The string representation of the candidate type.
 */
PJ_DECL(const char*) pj_ice_get_cand_type_name(pj_ice_cand_type type);


/**
 * This is a utility function to calculate the foundation identification
 * for a candidate.
 *
 * @param pool		Pool to allocate the foundation string.
 * @param foundation	Pointer to receive the foundation string.
 * @param type		Candidate type.
 * @param base_addr	Base address of the candidate.
 */
PJ_DECL(void) pj_ice_calc_foundation(pj_pool_t *pool,
				     pj_str_t *foundation,
				     pj_ice_cand_type type,
				     const pj_sockaddr *base_addr);


/**
 * Create ICE session with the specified role and number of components.
 * Application would typically need to create an ICE session before
 * sending an offer or upon receiving one. After the session is created,
 * application can register candidates to the ICE session by calling
 * #pj_ice_sess_add_cand() function.
 *
 * @param stun_cfg	The STUN configuration settings, containing among
 *			other things the timer heap instance to be used
 *			by the ICE session.
 * @param name		Optional name to identify this ICE instance in
 *			the log file.
 * @param role		ICE role.
 * @param comp_cnt	Number of components.
 * @param cb		ICE callback.
 * @param local_ufrag	Optional string to be used as local username to
 *			authenticate incoming STUN binding request. If
 *			the value is NULL, a random string will be 
 *			generated.
 * @param local_passwd	Optional string to be used as local password.
 * @param p_ice		Pointer to receive the ICE session instance.
 *
 * @return		PJ_SUCCESS if ICE session is created successfully.
 */
PJ_DECL(pj_status_t) pj_ice_sess_create(pj_stun_config *stun_cfg,
				        const char *name,
				        pj_ice_sess_role role,
				        unsigned comp_cnt,
				        const pj_ice_sess_cb *cb,
				        const pj_str_t *local_ufrag,
				        const pj_str_t *local_passwd,
				        pj_ice_sess **p_ice);

/**
 * Destroy ICE session. This will cancel any connectivity checks currently
 * running, if any, and any other events scheduled by this session, as well
 * as all memory resources.
 *
 * @param ice		ICE session instance.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_ice_sess_destroy(pj_ice_sess *ice);


/**
 * Change session role. This happens for example when ICE session was
 * created with controlled role when receiving an offer, but it turns out
 * that the offer contains "a=ice-lite" attribute when the SDP gets
 * inspected.
 *
 * @param ice		The ICE session.
 * @param new_role	The new role to be set.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error.
 */
PJ_DECL(pj_status_t) pj_ice_sess_change_role(pj_ice_sess *ice,
					     pj_ice_sess_role new_role);


/**
 * Assign a custom preference values for ICE candidate types. By assigning
 * custom preference value, application can control the order of candidates
 * to be checked first. The default preference settings is to use 126 for 
 * host candidates, 100 for server reflexive candidates, 110 for peer 
 * reflexive candidates, an 0 for relayed candidates.
 *
 * Note that this function must be called before any candidates are added
 * to the ICE session.
 *
 * @param ice		The ICE session.
 * @param prefs		Array of candidate preference value. The values are
 *			put in the array indexed by the candidate type as
 *			specified in pj_ice_cand_type.
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ice_sess_set_prefs(pj_ice_sess *ice,
					   const pj_uint8_t prefs[4]);



/**
 * Add a candidate to this ICE session. Application must add candidates for
 * each components ID before it can start pairing the candidates and 
 * performing connectivity checks.
 *
 * @param ice		ICE session instance.
 * @param comp_id	Component ID of this candidate.
 * @param type		Candidate type.
 * @param local_pref	Local preference for this candidate, which
 *			normally should be set to 65535.
 * @param foundation	Foundation identification.
 * @param addr		The candidate address.
 * @param base_addr	The candidate's base address.
 * @param rel_addr	Optional related address.
 * @param addr_len	Length of addresses.
 * @param p_cand_id	Optional pointer to receive the candidate ID.
 *
 * @return		PJ_SUCCESS if candidate is successfully added.
 */
PJ_DECL(pj_status_t) pj_ice_sess_add_cand(pj_ice_sess *ice,
					  unsigned comp_id,
					  pj_ice_cand_type type,
					  pj_uint16_t local_pref,
					  const pj_str_t *foundation,
					  const pj_sockaddr_t *addr,
					  const pj_sockaddr_t *base_addr,
					  const pj_sockaddr_t *rel_addr,
					  int addr_len,
					  unsigned *p_cand_id);

/**
 * Find default candidate for the specified component ID, using this
 * rule:
 *  - if the component has a successful candidate pair, then the
 *    local candidate of this pair will be returned.
 *  - otherwise a relay, reflexive, or host candidate will be selected 
 *    on that specified order.
 *
 * @param ice		The ICE session instance.
 * @param comp_id	The component ID.
 * @param p_cand_id	Pointer to receive the candidate ID.
 *
 * @return		PJ_SUCCESS if a candidate has been selected.
 */
PJ_DECL(pj_status_t) pj_ice_sess_find_default_cand(pj_ice_sess *ice,
						   unsigned comp_id,
						   int *p_cand_id);

/**
 * Pair the local and remote candidates to create check list. Application
 * typically would call this function after receiving SDP containing ICE
 * candidates from the remote host (either upon receiving the initial
 * offer, for UAS, or upon receiving the answer, for UAC).
 *
 * Note that ICE connectivity check will not start until application calls
 * #pj_ice_sess_start_check().
 *
 * @param ice		ICE session instance.
 * @param rem_ufrag	Remote ufrag, as seen in the SDP received from 
 *			the remote agent.
 * @param rem_passwd	Remote password, as seen in the SDP received from
 *			the remote agent.
 * @param rem_cand_cnt	Number of remote candidates.
 * @param rem_cand	Remote candidate array. Remote candidates are
 *			gathered from the SDP received from the remote 
 *			agent.
 *
 * @return		PJ_SUCCESS or the appropriate error code.
 */
PJ_DECL(pj_status_t) 
pj_ice_sess_create_check_list(pj_ice_sess *ice,
			      const pj_str_t *rem_ufrag,
			      const pj_str_t *rem_passwd,
			      unsigned rem_cand_cnt,
			      const pj_ice_sess_cand rem_cand[]);

/**
 * Start ICE periodic check. This function will return immediately, and
 * application will be notified about the connectivity check status in
 * #pj_ice_sess_cb callback.
 *
 * @param ice		The ICE session instance.
 *
 * @return		PJ_SUCCESS or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ice_sess_start_check(pj_ice_sess *ice);


/**
 * Send data using this ICE session. If ICE checks have not produced a
 * valid check for the specified component ID, this function will return
 * with failure. Otherwise ICE session will send the packet to remote
 * destination using the nominated local candidate for the specified
 * component.
 *
 * This function will in turn call \a on_tx_pkt function in
 * #pj_ice_sess_cb callback to actually send the packet to the wire.
 *
 * @param ice		The ICE session.
 * @param comp_id	Component ID.
 * @param data		The data or packet to be sent.
 * @param data_len	Size of data or packet, in bytes.
 *
 * @return		PJ_SUCCESS if data is sent successfully.
 */
PJ_DECL(pj_status_t) pj_ice_sess_send_data(pj_ice_sess *ice,
					   unsigned comp_id,
					   const void *data,
					   pj_size_t data_len);

/**
 * Report the arrival of packet to the ICE session. Since ICE session
 * itself doesn't have any transports, it relies on application or
 * higher layer component to give incoming packets to the ICE session.
 * If the packet is not a STUN packet, this packet will be given back
 * to application via \a on_rx_data() callback in #pj_ice_sess_cb.
 *
 * @param ice		The ICE session.
 * @param comp_id	Component ID.
 * @param pkt		Incoming packet.
 * @param pkt_size	Size of incoming packet.
 * @param src_addr	Source address of the packet.
 * @param src_addr_len	Length of the address.
 *
 * @return		PJ_SUCCESS or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ice_sess_on_rx_pkt(pj_ice_sess *ice,
					   unsigned comp_id,
					   void *pkt,
					   pj_size_t pkt_size,
					   const pj_sockaddr_t *src_addr,
					   int src_addr_len);



/**
 * @}
 */


PJ_END_DECL


#endif	/* __PJNATH_ICE_SESSION_H__ */

