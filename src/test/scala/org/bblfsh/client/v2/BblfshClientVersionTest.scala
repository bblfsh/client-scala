package org.bblfsh.client.v2

import gopkg.in.bblfsh.sdk.v2.protocol.driver.VersionResponse
import org.scalatest.{BeforeAndAfter, BeforeAndAfterAll, FunSuite}

import scala.io.Source

class BblfshClientVersionTest extends FunSuite
  with BeforeAndAfter with BeforeAndAfterAll {
  val client = BblfshClient("localhost", 9432)
  val fileName = "src/test/resources/SampleJavaFile.java"
  val fileContent = Source.fromFile(fileName).getLines.mkString("\n")
  var resp: VersionResponse = _

  before {
    resp = client.version()
  }

  override def afterAll {
    client.close()
  }

  test("Check version") {
    assert(!resp.version.isEmpty)
  }

}
