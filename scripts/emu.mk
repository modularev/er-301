ARCH=linux
include scripts/env.mk
include scripts/utils.mk

program_name := emu
program_dir := $(program_name)
out_dir := $(build_dir)/$(program_name)

src_dirs := $(program_dir) $(hal_dir) $(arch_hal_dir) $(od_dir) $(ti_dir)
includes += . $(arch_dir) $(lua_dir) $(lodepng_dir) $(miniz_dir) $(libs_dir)/SDL_FontCache

libraries :=
libraries += $(libs_build_dir)/lib$(lua_name).a
libraries += $(libs_build_dir)/liblodepng.a
libraries += $(libs_build_dir)/libminiz.a

# Recursive search for source files
cpp_sources := $(foreach D,$(src_dirs),$(call rwildcard,$D,*.cpp))
c_sources := $(foreach D,$(src_dirs),$(call rwildcard,$D,*.c))

objects := $(addprefix $(out_dir)/,$(c_sources:%.c=%.o) $(cpp_sources:%.cpp=%.o))

# Manually add objects
objects += $(out_dir)/od/glue/app_swig.o
objects += $(out_dir)/libs/SDL_FontCache/SDL_FontCache.o

CFLAGS += -DFIRMWARE_VERSION=\"$(FIRMWARE_VERSION)\"
CFLAGS += -DFIRMWARE_NAME=\"$(FIRMWARE_NAME)\"
CFLAGS += -DFIRMWARE_STATUS=\"$(FIRMWARE_STATUS)\"
LFLAGS += -lSDL2 -lSDL2_ttf -lfftw3f -lm -ldl -lstdc++

all: $(out_dir)/$(program_name).elf

$(out_dir)/$(program_name).elf: $(objects) $(libraries)
	@mkdir -p $(@D)
	@echo $(describe_env) LINK $(describe_target)
	@$(CC) $(CFLAGS) -o $@ $(objects) $(libraries) $(LFLAGS)

clean:
	rm -rf $(out_dir)

addr2line: $(out_dir)/$(program_name).elf
	@echo $(describe_env) Find ${ADDRESS} in $(out_dir)/$(program_name).elf
	@$(ADDR2LINE) -p -f -i -C -e $(out_dir)/$(program_name).elf -a $(ADDRESS)

missing: $(objects) $(libraries)
	@echo $(describe_env) Generating list of missing references...
	-@$(CC) $(CFLAGS) -o $@ $(objects) $(libraries) $(LFLAGS) 2> $(out_dir)/error.log
	@$(PYTHON) list-undefined.py $(out_dir)/error.log > $(out_dir)/missing.log

libs:
	+$(MAKE) -f lua.mk ARCH=linux
	+$(MAKE) -f miniz.mk ARCH=linux
	+$(MAKE) -f lodepng.mk ARCH=linux

clean-libs:
	+$(MAKE) -f lua.mk clean ARCH=linux
	+$(MAKE) -f miniz.mk clean ARCH=linux
	+$(MAKE) -f lodepng.mk clean ARCH=linux

include scripts/rules.mk
