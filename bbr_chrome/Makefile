CUR_DIR=$(shell pwd)
OUTDIR:=$(CUR_DIR)/out/
BIN_DIR:=$(CUR_DIR)/bin/
HEADPATH:=$(CUR_DIR)
CFLAGS:=-g -c
VPATH=$(shell find src -type d)# recurive path search need help from shell
CXX=g++
TARGET=$(BIN_DIR)client_test $(BIN_DIR)server_test

RED = \\e[1m\\e[31m
RESET = \\e[m

DEPS:=client.o\
server.o\
packet_maker.o\
packet.o\
utils.o\
time.o\
send_algorithm_interface.o\
chrome_bbr_sender.o\
rtt_stats.o\
packet_parser.o\
general_loss_algorithm.o\
bandwidth_sampler.o\
quic_chromium_clock.o\
quic_packet_number.o\
quic_clock.o\
quic_time.o\
quic_sent_packet_manager.o\
quic_constants.o\
quic_unacked_packet_map.o\
quic_transmission_info.o\
bbr2_drain.o\
bbr2_misc.o\
bbr2_probe_bw.o\
bbr2_probe_rtt.o\
bbr2_sender.o\
bbr2_startup.o\
quic_bandwidth.o\
cubic_bytes.o\
hybrid_slow_start.o\
tcp_cubic_sender_bytes.o\
prr_sender.o\


DEPS2=$(patsubst %.o,$(OUTDIR)%.o,$(DEPS) )

client_test:$(BIN_DIR)client_test ; #default is client_test

server_test:$(BIN_DIR)server_test ;

quic_sent_packet_manager_test:$(BIN_DIR)quic_sent_packet_manager_test ;

$(BIN_DIR)client_test:$(DEPS2)  $(OUTDIR)client_test.o
	$(CXX) $^ -o $@ -I $(HEADPATH)
	@echo $(RED) $@ build Done! $(RESET)

$(BIN_DIR)server_test:$(DEPS2) $(OUTDIR)server_test.o
	$(CXX) $^ -o $@ -I $(HEADPATH)
	@echo $(RED) $@ build Done! $(RESET)

$(BIN_DIR)quic_sent_packet_manager_test:$(DEPS2) $(OUTDIR)quic_sent_packet_manager_test.o
	$(CXX) $^ -o $@ -I $(HEADPATH)
	@echo $(RED) $@ build Done! $(RESET)

$(OUTDIR)%.o:%.cc # .cc file need be found in VPATH
	$(CXX) $(CFLAGS) $^ -o $@ -I $(HEADPATH)

.PHONY:clean
clean:
	rm $(DEPS2) $(TARGET) $(OUTDIR)client_test.o $(OUTDIR)server_test.o
