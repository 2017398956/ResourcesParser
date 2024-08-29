rp : \
	main.cpp \
	ResourcesParserInterpreter.h \
	ResourcesParserInterpreter.cpp \
	ResourcesParser.h \
	ResourcesParser.cpp \
	android/ResourceTypes.h \
	android/ResourceTypes.cpp \
	android/configuration.h \
	android/ByteOrder.h
	g++ main.cpp ResourcesParserInterpreter.cpp ResourcesParser.cpp android/ResourceTypes.cpp -std=c++11 -o rp

test : \
	Test.cpp \
	ParseXML.h \
	ParseXML.cpp \
	ParseArsc.h \
	ParseArsc.cpp \
	ParseUtil.h \
	ParseUtil.cpp \
	android/ResourceTypes.h \
	android/ResourceTypes.cpp \
	android/configuration.h \
	android/ByteOrder.h
	g++ Test.cpp ParseXML.cpp ParseArsc.cpp ParseUtil.cpp android/ResourceTypes.cpp -std=c++11 -o test

.PHONY : clean
clean :
	rm rp

