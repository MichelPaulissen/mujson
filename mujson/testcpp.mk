##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=testcpp
ConfigurationName      :=Debug
WorkspacePath          := "/mnt/3CE6799F208B305A/mujson/mujson"
ProjectPath            := "/mnt/3CE6799F208B305A/mujson/mujson"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Michel Paulissen
Date                   :=02/14/14
CodeLitePath           :="/home/michel/.codelite"
LinkerName             :=g++
SharedObjectLinkerName :=g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-gstab
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)MUJSON_USE_CPP_INTERFACE 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="testcpp.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch).. 
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
CXX      := g++
CC       := gcc
CXXFLAGS :=  -g -O0 -Wall -Werror $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall -Werror $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) $(IntermediateDirectory)/mujson_test$(ObjectSuffix) 



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
$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix): ../mujson.cpp $(IntermediateDirectory)/mujson_mujson$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/mnt/3CE6799F208B305A/mujson/mujson.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mujson_mujson$(DependSuffix): ../mujson.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mujson_mujson$(ObjectSuffix) -MF$(IntermediateDirectory)/mujson_mujson$(DependSuffix) -MM "../mujson.cpp"

$(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix): ../mujson.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix) "../mujson.cpp"

$(IntermediateDirectory)/mujson_test$(ObjectSuffix): ../test.cpp $(IntermediateDirectory)/mujson_test$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/mnt/3CE6799F208B305A/mujson/test.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mujson_test$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mujson_test$(DependSuffix): ../test.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mujson_test$(ObjectSuffix) -MF$(IntermediateDirectory)/mujson_test$(DependSuffix) -MM "../test.cpp"

$(IntermediateDirectory)/mujson_test$(PreprocessSuffix): ../test.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mujson_test$(PreprocessSuffix) "../test.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/mujson_mujson$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/mujson_mujson$(DependSuffix)
	$(RM) $(IntermediateDirectory)/mujson_mujson$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/mujson_test$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/mujson_test$(DependSuffix)
	$(RM) $(IntermediateDirectory)/mujson_test$(PreprocessSuffix)
	$(RM) $(OutputFile)
	$(RM) ".build-debug/testcpp"


