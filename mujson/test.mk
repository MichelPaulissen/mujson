##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=test
ConfigurationName      :=Debug
WorkspacePath          := "/mnt/3CE6799F208B305A/mujson/mujson"
ProjectPath            := "/mnt/3CE6799F208B305A/mujson/mujson"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Michel Paulissen
Date                   :=03/11/14
CodeLitePath           :="/home/michel/.codelite"
LinkerName             :=gcc
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="test.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)../ 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := gcc
CC       := gcc
CXXFLAGS :=  -g -O0 -Wall -Werror -Wno-unused-function $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall -Werror -Wno-unused-function -fbounds-check -std=c99 $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/mujson_test$(ObjectSuffix) $(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/mujson_test$(ObjectSuffix): ../test.c $(IntermediateDirectory)/mujson_test$(DependSuffix)
	$(CC) $(SourceSwitch) "/mnt/3CE6799F208B305A/mujson/test.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mujson_test$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mujson_test$(DependSuffix): ../test.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mujson_test$(ObjectSuffix) -MF$(IntermediateDirectory)/mujson_test$(DependSuffix) -MM "../test.c"

$(IntermediateDirectory)/mujson_test$(PreprocessSuffix): ../test.c
	@$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mujson_test$(PreprocessSuffix) "../test.c"

$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix): ../mujson.c $(IntermediateDirectory)/mujson_mujson$(DependSuffix)
	$(CC) $(SourceSwitch) "/mnt/3CE6799F208B305A/mujson/mujson.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mujson_mujson$(DependSuffix): ../mujson.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) -MF$(IntermediateDirectory)/mujson_mujson$(DependSuffix) -MM "../mujson.c"

$(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix): ../mujson.c
	@$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix) "../mujson.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/mujson_test$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/mujson_test$(DependSuffix)
	$(RM) $(IntermediateDirectory)/mujson_test$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/mujson_mujson$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/mujson_mujson$(DependSuffix)
	$(RM) $(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix)
	$(RM) $(OutputFile)
	$(RM) ".build-debug/test"


