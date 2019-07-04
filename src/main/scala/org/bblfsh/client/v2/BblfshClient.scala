package org.bblfsh.client.v2

import java.nio.ByteBuffer

import com.google.protobuf.ByteString
import gopkg.in.bblfsh.sdk.v2.protocol.driver._
import io.grpc.ManagedChannelBuilder
import org.bblfsh.client.v2.libuast.Libuast


class BblfshClient(host: String, port: Int, maxMsgSize: Int) {
  private val DEFAULT_TIMEOUT_SEC = 5

  private val channel = ManagedChannelBuilder
    .forAddress(host, port)
    .usePlaintext(true)
    .maxInboundMessageSize(maxMsgSize)
    .build()

  private val stub = DriverGrpc.blockingStub(channel)
  private val stubInfo = DriverHostGrpc.blockingStub(channel)

  /**
    * Parses file with a given name and content using 
    * the provided timeout.
    *
    * @param name    file name
    * @param content file content
    * @param lang    (optional) language to parse, default auto-detect \w enry
    * @param timeout (disabled) bblfsh request timeout, seconds
    *                Right now this does not have any effect in v2.
    * @return UAST in parse response.
    */
  def parseWithOptions(
    name: String,
    content: String,
    lang: String,
    timeout: Long
  ): ParseResponse = {
    // FIXME(bzz): make timout work in v2 again
    val req = ParseRequest(
      filename = name,
      content = content,
      language = lang
    )
    stub.parse(req)
  }

  /**
    * Parses the given file name and content.
    *
    * A default timeout of 5 seconds will be applied, same as was done by the server
    * before https://github.com/bblfsh/bblfshd/blob/83166ea0087bfe20c24fc471309f70f422383198/daemon/pool.go#L191
    *
    * @param name    file name
    * @param content file content
    * @param lang    (optional) language to parse, default auto-detect \w enry
    * @return UAST in parse response.
    */
  def parse(
    name: String,
    content: String,
    lang: String = ""
  ): ParseResponse = parseWithOptions(name, content, lang, DEFAULT_TIMEOUT_SEC)

  /**
    * Parses the given file name and content,
    * setting a bblfsh request timeout.
    *
    * Since v1.11, this API exposes the timeout.
    *
    * @param name    file name
    * @param content file content
    * @param timeout bblfsh request timeout, seconds
    * @param lang    (optional) language to parse, default auto-detect \w enry
    * @return
    */
  def parseWithTimeout(
    name: String,
    content: String,
    timeout: Long,
    lang: String = ""
  ): ParseResponse = parseWithOptions(name, content, lang, timeout)

  def supportedLanguages(): SupportedLanguagesResponse = {
    val req = SupportedLanguagesRequest()
    stubInfo.supportedLanguages(req)
  }

  def version(): VersionResponse = {
    val req = VersionRequest()
    stubInfo.serverVersion(req)
  }

  def close(): Unit = {
    channel.shutdownNow()
  }
}

object BblfshClient {
  val DEFAULT_MAX_MSG_SIZE = 100 * 1024 * 1024 // bytes

  // TODO(bzz): expose new 'children' order and use enum/case class
  val PreOrder = 0
  val PostOrder = 1
  val LevelOrder = 2
  val PositionOrder = 3

  private val libuast = new Libuast

  def apply(
    host: String, port: Int,
    maxMsgSize: Int = DEFAULT_MAX_MSG_SIZE
  ): BblfshClient = new BblfshClient(host, port, maxMsgSize)

  def filter(node: Node, query: String): List[Node] = Libuast.synchronized {
    libuast.filter(node, query)
  }

  /**
    * Decodes bytes from wired format of bblfsh protocol.v2.
    * Requires a buffer in Direct mode.
    *
    * Since v2.
    */
  def decode(buf: ByteBuffer): ContextExt = Libuast.synchronized {
    if (!buf.isDirect()) {
      throw new RuntimeException("Only directly-allocated buffer decoding is supported.")
    }
    libuast.decode(buf)
  }

  // Enables API: resp.uast.decode()
  implicit class UastMethods(val buf: ByteString) {
    /**
      * Decodes bytes from wired format of bblfsh protocol.v2.
      * Copies a buffer in Direct mode.
      */
    def decode(): ContextExt = {
      val bufDirectCopy = ByteBuffer.allocateDirect(buf.size)
      buf.copyTo(bufDirectCopy)
      BblfshClient.decode(bufDirectCopy)
    }
  }
  
  // TODO(bzz): implement
  // def iterator(node: Node, treeOrder: Int): Libuast.UastIterator = {
  //   libuast.iterator(node, treeOrder)
  // }

  // implicit class NodeMethods(val node: Node) {
  //   def filter(query: String): List[Node] = {
  //     BblfshClient.filter(node, query)
  //   }
  // }


}

