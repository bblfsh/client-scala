package org.bblfsh.client.v2

import gopkg.in.bblfsh.sdk.v2.protocol.driver.SupportedLanguagesResponse
import org.scalatest.{BeforeAndAfter, FunSuite}

// Depends on having bblfshd JavaScript drivers running
class BblfshClientSupportedLanguagesTest extends FunSuite with BeforeAndAfter {
  val client = BblfshClient("localhost", 9432)
  var resp: SupportedLanguagesResponse = _

  before {
    resp = client.supportedLanguages()
  }

  test("Check languages are not empty") {
    assert(!resp.languages.isEmpty)
  }

  test("Check languages contain JavaScript") {
    assert(resp.languages.map(_.name).contains("JavaScript"))
  }

  test("Check languages include aliases") {
    println(resp.languages.flatMap(_.aliases).contains("JSX"))
  }
}
