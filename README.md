## Babelfish Scala client [![Build Status](https://travis-ci.org/bblfsh/client-scala.svg?branch=master)](https://travis-ci.org/bblfsh/client-scala)

This a Scala/JNI implementation of the [Babelfish](https://doc.bblf.sh/) client.
It uses [ScalaPB](https://scalapb.github.io/grpc.html) for Protobuf/gRPC code
generation and [libuast](https://github.com/bblfsh/libuast) for XPath queries.

### Status

Current `scala-client` v1.x only supports bblfsh protocol and UASTv1.

### Installation

#### Manual
```
git clone https://github.com/bblfsh/client-scala.git
cd client-scala
./sbt assembly
```

gRPC/protobuf files are re-generate from `src/main/proto` on every `./sbt compile`
and are stored under `./target/src_managed/`. 

The jar file and the native module are generated in the `build/` directory. If 
you move the jar file to some other path, the native (`.so` or `.dylib`) 
library must be in the same path.

If the build fails because it can't find the `jni.h` header file, run it with:

```
./sbt -java-home /usr/lib/jvm/java-8-openjdk-amd64 assembly
```

Changing the JDK directory to the one right for your system.

#### Apache Maven

The `bblfsh-client` package is available thorugh [Maven
central](http://search.maven.org/#search%7Cga%7C1%7Cbblfsh), so it can be easily
added as a dependency in various package management systems.  Examples of how to
handle it for most commons systems are included below; for other systems just look
at Maven central's dependency information.

```xml
<dependency>
    <groupId>org.bblfsh</groupId>
    <artifactId>bblfsh-client</artifactId>
    <version>${version}</version>
</dependency>
```

#### Scala sbt

```
libraryDependencies += "org.bblfsh" % "bblfsh-client" % version
```

### Dependencies

You need to install libxml2, the Java SDK and its header files. The command for 
Debian and derived distributions would be:

```
sudo apt install libxml2-dev openjdk-8 openjdk-8-jdk-headless
```

### Usage

If you don't have a bblfsh server running you can execute it using the following
command:

```
docker run --privileged --rm -it -p 9432:9432 --name bblfsh bblfsh/bblfshd
```   

Please, read the [getting started](https://doc.bblf.sh/using-babelfish/getting-started.html)
guide to learn more about how to use and deploy a bblfsh server.

API
```scala
import scala.io.Source
import org.bblfsh.client.BblfshClient

val client = BblfshClient("0.0.0.0", 9432)

val filename = "/path/to/file.py" // client responsible for encoding it to utf-8
val fileContent = Source.fromFile(filename).getLines.mkString("\n")
val resp = client.parse(filename, fileContent)

// Full response
println(resp.uast.get)

// Filtered response
println(client.filter(resp.uast.get, "//Import[@roleImport]"))

// Filtered responses using XPath functions returning types
// other than NodeLists (Bool, Number, String):
println(client.filterBool(resp.uast.get, "boolean(//*[@strtOffset or @endOffset])"))
println(client.filterString(resp.uast.get, "name(//*[1])"))
println(client.filterNumber(resp.uast.get, "count(//*)"))
```

Command line:

```
java -jar build/bblfsh-client-assembly-*.jar -f <file.py>
```

or if you want to use a XPath query:

```
java -jar build/bblfsh-client-assembly-*.jar -f <file.py> -q "//Import[@roleImport]"
```

Please read the [Babelfish clients](https://doc.bblf.sh/user/language-clients.html)
guide section to learn more about babelfish clients and their query language.

### License

Apache 2.0
