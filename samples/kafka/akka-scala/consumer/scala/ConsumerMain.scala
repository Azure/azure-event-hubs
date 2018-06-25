import akka.actor.ActorSystem
import akka.kafka.scaladsl.Consumer
import akka.kafka.{ConsumerSettings, Subscriptions}
import akka.stream.ActorMaterializer
import akka.stream.scaladsl.Sink
import org.apache.kafka.common.serialization.{ByteArrayDeserializer, StringDeserializer}

import scala.concurrent.Future
import scala.language.postfixOps

object ConsumerMain {

  def main(args: Array[String]): Unit = {
    implicit val system:ActorSystem = ActorSystem.apply("akka-stream-kafka")
    implicit val materializer:ActorMaterializer = ActorMaterializer()

    // grab our settings from the resources/application.conf file
    val consumerSettings = ConsumerSettings(system, new ByteArrayDeserializer, new StringDeserializer)

    // our topic to subscribe to for messages
    val topic = "test"

    // listen to our topic with our settings, until the program is exited
    Consumer.plainSource(consumerSettings, Subscriptions.topics(topic))
      .mapAsync(1) ( msg => {
        // print out our message once it's received
        println(s"Message Received : ${msg.timestamp} - ${msg.value}")
        Future.successful(msg)
      }).runWith(Sink.ignore)
  }
}