AUTOMAKE_OPTIONS = foreign
SUBDIRS = src
CURRENTPATH=$(shell /bin/pwd)

gz: all
	@rm -rf $(GZ_TARGET_FILE)
	@rm -rf $(GZ_TARGET_DIR)
	@mkdir -p $(GZ_TARGET_DIR)
	@mkdir -p $(GZ_TARGET_DIR)/ppd
	@cp ./install-driver $(GZ_TARGET_DIR)/
	@cp ./uninstall-driver $(GZ_TARGET_DIR)/
	@cp ./ppd/*.ppd $(GZ_TARGET_DIR)/ppd/
	@cp ./src/$(FILTER_PROGRAMS) $(GZ_TARGET_DIR)/
	@chown -R root:root $(GZ_TARGET_DIR)/*
	@chmod 744 $(GZ_TARGET_DIR)/install-driver $(GZ_TARGET_DIR)/uninstall-driver
	tar -czvf $(GZ_TARGET_FILE) $(GZ_TARGET_DIR)
