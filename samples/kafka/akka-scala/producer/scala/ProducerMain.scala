import java.util.concurrent.TimeUnit.SECONDS

import akka.actor.ActorSystem
import akka.kafka.ProducerSettings
import akka.kafka.scaladsl.Producer
import akka.stream.scaladsl.Source
import akka.stream.{ActorMaterializer, ThrottleMode}
import org.apache.kafka.clients.producer.ProducerRecord
import org.apache.kafka.common.serialization.{ByteArraySerializer, StringSerializer}

import scala.concurrent.duration.FiniteDuration
import scala.language.postfixOps

object ProducerMain {

  def main(args: Array[String]): Unit = {
    implicit val system: ActorSystem = ActorSystem.apply("akka-stream-kafka")
    implicit val materializer: ActorMaterializer = ActorMaterializer()

    // grab our settings from the resources/application.conf file
    val producerSettings = ProducerSettings(system, new ByteArraySerializer, new StringSerializer)

    // topic to send the message to on event hubs
    val topic = "sampleTopic"

    // loop until the program reaches 100
    Source(1 to 100)
      .throttle(1, FiniteDuration(1, SECONDS), 1, ThrottleMode.Shaping)
      .map(num => {
        //construct our message here
        val message = s"Akka Scala Producer Message # ${num}"
        println(s"Message sent to topic - $topic - $message")
        new ProducerRecord[Array[Byte], String](topic, message.getBytes, message.toString)
      })
      .runWith(Producer.plainSink(producerSettings))
      .onComplete(_ => {
        println("All messages sent!")
        system.terminate()
      })(scala.concurrent.ExecutionContext.global)
  }
}