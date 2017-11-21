#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <queue>

using namespace std;
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

//timeOut - since average timeout is 5 time unit, therefore timeout should be more than 2RTT (i.e >10)
#define timeOut 10.5

//payloadDataSize - static variable for payload size
#define payloadDataSize 20

//seqNumA - sequence number sent from A to B
int seqNumA = 0;

//ackNumA - acknowledge number received from B to A
int ackNumA = 0;

//seqNumB - sequence number sent from B to A
int seqNumB = 0;

//ackNumB - acknowledge number received from A to B
int ackNumB = 0;

//messageBuffer - buffer maintained for message in transition
queue<msg> messageBuffer;

//inTransition - variable which indicates that message is in transition from A to B
bool inTransition = false;

//isFirstTimePacket - variable which indicated that packet is First Time Packet
bool isFirstTimePacket = true;

//currentMessage - currentMessage
struct msg currentMessage;

//currentPacket - currentPacket
struct pkt currentPacket;

//flipSeqNumA - function to flip seq number of A
void flipSeqNumA(){ seqNumA = (seqNumA+1)%2; }

//flipAckNumA - function to Ack number of A
void flipAckNumA(){ ackNumA = (ackNumA+1)%2; }

//function to flip seq number of B
void flipSeqNumB(){ seqNumB = (seqNumB+1)%2; }

//flipAckNumB - function to Ack number of A
void flipAckNumB(){ ackNumB = (ackNumB+1)%2; }

/*getCheckSum - generating checksum for the packet
     Referred from : https://docs.google.com/document/u/1/d/19I8-TrLNcfaCGX1L-KSx5xFYEoiFAN3F9o_jQlOgsFM/pub
     Algorithm :
         TCP-like checksum, which consists of the sum of the (integer) sequence and ack field values,
         added to a character-by-character sum of the payload field of the packet
*/
int getCheckSum(struct pkt packet){
    int checkSumValue = 0;

    checkSumValue += packet.seqnum;
    checkSumValue += packet.acknum;

    for(int i=0;i<payloadDataSize;i++){
        checkSumValue += (int)packet.payload[i];
    }
    return checkSumValue;
}


//isEmptyBuffer - function to check if message buffer is empty or not
bool isEmptyBuffer(){
    if(!messageBuffer.empty())
        return true;
    else
        return false;
}

//getPacketInfo - function get Packet Info
void getPacketInfo(int AorB, struct pkt packet){
    string AorBString;
    if(AorB == 0)
        AorBString = "A";
    else
        AorBString  = "B";
    cout << "------------------------------------------------" << endl;
    cout << AorBString << " seqnum =  " << packet.seqnum  << "                                  |" << endl;
    cout << AorBString << " acknum =  " << packet.acknum <<  "                                  |"<<  endl;
    cout << AorBString <<" payload =  " << packet.payload << "              |"<< endl;
    cout << AorBString << " checksum =  " << packet.checksum << "                             |" <<endl;
    cout << "------------------------------------------------" << endl;
}

/*A_output - called from layer 5, passed the data to be sent to other side
 Objective : data to be sent to the B-side
 Edge cases : message is delivered in-order, and correctly, to the receiving side upper layer.
    Algorithm:
     fill in the payload field from the message data passed down from Layer 5
        1.  Initialize a empty packet
        2.  Prepare data from msg
            2.1 copy message data to the payload of packet to send.
            2.2 add sending Sequence Number to packet (initially 0)
            2.3 add receiving ack Number to packet (initially 0)
            2.4 add checksum to the packet
        3. Check is packet is sent for the very first time
            3.1 If yes, add message to the buffer
            3.2 Else, do not add message in buffer (since it is already present in buffer)
            3.3 Send packet to layer3
            3.4 Start time out timer to timeOut (10.5)
 * */
void A_output(struct msg message) {

    currentMessage = message;

    /*Initializing the packet that needs to be send to layer 3*/
    struct pkt packetToSend;

    /*Preparing data to send the the layer 3*/

    /*copying message data to packets' payload*/
    strncpy(packetToSend.payload,message.data,payloadDataSize);

    /*adding sequence to the packet.*/
    packetToSend.seqnum = seqNumA;

    /*adding ack number to the packet.*/
    packetToSend.acknum = ackNumB;

    /*add checksum to the packet*/
    packetToSend.checksum = getCheckSum(packetToSend);

    //copying packet to variable for later use
    currentPacket = packetToSend;

    //if packet is sent for first time, add to queue, modify inTransition accordingly
    if (isFirstTimePacket) {
        messageBuffer.push(message);
        if (!inTransition) {
            inTransition = true;
            /*3. Send packet to layer3*/
            tolayer3(0, packetToSend);
            starttimer(0, timeOut);
        }
    //no need to add packet to queue, if it already exist
    } else {
        isFirstTimePacket = true;
        inTransition = true;
        /*3. Send packet to layer3*/
        tolayer3(0, packetToSend);
        starttimer(0, timeOut);
    }
}


/* called from layer 3, when a packet arrives for layer 4
 Objective : receive packet : this is called whenever a packet sent from the B-side (Or when B calls toLayer3(), just like opposite of A_output)
 check the expected packet, corrupt or not !
 Edge case: NONE (packet is the (possibly corrupted) packet sent from the B-side.), just recieve the packet
 Algo:
    1. Create a dummy packet(aka return packet)
    2. add checksum same as input checksum
    3. If input ackNumber with prev ackNumA && inputSequenceNumber = prev seq no. (This means packet is received properly)
        2.1 flip seqNumA
        2.2 reset timer / stop timer,iteration of simulation is done !
        2.3 remove top message from the buffer
        2.4 if buffer is non empty
            2.4.1 send dummy packet to layer 3 and start timer

 * */
void A_input(struct pkt packet) {

 // cout << "\n33333333333333333333333 Inside A_input packet received \n";

    //returnPacket - packet returned after is input packet is received
    struct pkt returnPacket;
    returnPacket.seqnum = seqNumA;
    returnPacket.acknum = ackNumA;

    int inputSequenceNumber = packet.seqnum;
    int inputAckNumber = packet.acknum;

    //getting message which is in front of the queue
    struct msg m = messageBuffer.front();
    strncpy(returnPacket.payload,m.data,payloadDataSize);
    packet.checksum = getCheckSum(packet);

    if (inputAckNumber == ackNumA && inputSequenceNumber == seqNumA){
        inTransition = false;
        flipAckNumA();
        flipSeqNumA();
        stoptimer(0);
        messageBuffer.pop();
        if (!(messageBuffer.empty())) {
            isFirstTimePacket = true;
            inTransition = true;
            tolayer3(0, returnPacket);
            starttimer(0, timeOut);
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt() {
    isFirstTimePacket = false;
    A_output(messageBuffer.front());
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(){
    // cout << "\n Inside A_init()\n";
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B
 Objective : receive packet from A, whenever A calls tolayer3()
 Ref: https://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f17/material/recs/rec3.pdf
 Algo:
    1. verify checkSum , use function computeChecksum(packet)
        1.1 If checkSum != expectedChecksum return
    2.If input seqNumber with prev seqNumB and ack number same as ackNumB
        2.1 flip seqNumB
        2.2 flip ackNumB
        2.2 send packet to layer 3
        2.3 send payload data to layer 5
 */
void B_input(struct pkt packet) {

    int inputPacketCheckSum = packet.checksum;
    int inputSequenceNumber = packet.seqnum;
    int inputAckNumber = packet.acknum;
    packet.checksum = 0;

    /*checking  inputPacketCheckSum with expected checksum from the packet*/
    if (inputPacketCheckSum != getCheckSum(packet)) {
        cout << "\n expectedCheckSum is not same as inputPacketCheckSum. Packet might be corrupt | Sim Time : " << get_sim_time() << endl;
        return;
    } else {
        if (inputSequenceNumber == seqNumB && inputAckNumber == ackNumB) {
            flipSeqNumB();
            flipAckNumB();
            tolayer5(1, packet.payload);
            tolayer3(1, packet); //send packet to layer 3 with the acknowledgement equal to prev seqNo.
        }
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    //cout << "\n Inside B_init()\n";
}