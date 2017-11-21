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
//timeOut - making is siffciently large,
#define timeOut 15

//payloadDataSize - static variable for payload size
#define payloadDataSize 20

//seqNumA - sequence number sent from A to B
int seqNumA = 0;

//ackNumA - acknowledge number received from B to A
int ackNumA = 0;

//seqNumB - sequence number sent from B to A
int seqNumB = 1;

//ackNumB - acknowledge number received from A to B
int ackNumB = 0;

//default window size
int windowSize = 0;

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

//packetFromB - packetFromB which contains the ACK
struct pkt packetFromB;


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


//buffer for GBN
static struct pkt sndpkt[1010];

int base = 1;
int nextSeqNum = 1;
int currentPointer = -1;


/* called from layer 5, passed the data to be sent to other side
    if (nextseqnum < base+N) {
    sndpkt[nextseqnum] = make_pkt(nextseqnum,data,chksum) udt_send(sndpkt[nextseqnum])
    if (base == nextseqnum)
    start_timer nextseqnum++ }
    else refuse_data(data)

 * */
int p=1;

void A_output(struct msg message) {

    cout << "\n Packet No. >---------------->>>>> " << p << " Sending Time .  " << get_sim_time() << endl;
    p++;

    cout << "Window Size = " << getwinsize() << endl;
    currentMessage = message;


    if(nextSeqNum < base + windowSize){

        /*Initializing the packet that needs to be send to layer 3*/
        struct pkt packetToSend;
        packetToSend.acknum = ackNumA;
        packetToSend.seqnum = seqNumA;
        packetToSend.checksum = getCheckSum(packetToSend);

        // add this packet to the buffer, sndpkt (index specified with nextSeqNum)
        sndpkt[nextSeqNum] = packetToSend;
        tolayer3(0,sndpkt[nextSeqNum]);

        if(base == nextSeqNum){
            starttimer(0,timeOut);
        }
        //incrementing for next packet in the buffer
        nextSeqNum++;

    }
//    else{
//        // refuse data
//    }
}

/* called from layer 3, when a packet arrives for layer 4

 rdt_rcv(rcvpkt) && notcorrupt(rcvpkt)
base = getacknum(rcvpkt)+1
 If (base == nextseqnum)
    stop_timer
 else
    restart_timer

 * */
void A_input(struct pkt packet) {

    int currentCheckSum = packet.checksum;

    // rdt_rcv(rcvpkt) && notcorrupt(rcvpkt) - check for non corrupt packet

    //modify base acc to : base = getacknum(rcvpkt)+1
    base = seqNumA + 1;

    packet.checksum = getCheckSum(packet);

    if(base == nextSeqNum && packet.checksum == currentCheckSum){
        stoptimer(0);
    }else{
        starttimer(0,timeOut);
    }
}

/* called when A's timer goes off
    timeout start_timer
    udt_send(sndpkt[base])
    udt_send(sndpkt[base+1])
    ...
    udt_send(sndpkt[nextseqnum-1])

 * */
void A_timerinterrupt() {
    starttimer(0,timeOut);

    //send all packets to layer 3 from buffer
    for(int i=base;i<nextSeqNum;i++){
        tolayer3(0,sndpkt[i]);
    }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B

 rdt_rcv(rcvpkt) && notcurrupt(rcvpkt) && hasseqnum(rcvpkt,expectedseqnum)

    extract(rcvpkt,data)
    deliver_data(data)
    sndpkt = make_pkt(expectedseqnum,ACK,chksum)
    udt_send(sndpkt)
    expectedseqnum++

 * */
void B_input(struct pkt packet) {
    packet.checksum = getCheckSum(packet);
    int currentSeqNum = packet.seqnum;
    int currentCheckSum= packet.checksum;

    //making packet to be delivered to A . assigning checksum to packet to be send back to A from B
    packetFromB.checksum = getCheckSum(packet);
    for (int i = 0; i < 20; ++i) {
        packetFromB.payload[i] = packet.payload[i];
    }

    if(currentSeqNum == seqNumB && currentCheckSum == sndpkt[seqNumB].checksum){
        tolayer5(1,packet.payload);
        packetFromB.acknum = seqNumA;
        tolayer3(1,packetFromB);
        seqNumB++;
    }


}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    packetFromB.acknum = seqNumB;
}
