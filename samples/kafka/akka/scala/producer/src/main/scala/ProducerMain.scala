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
import scala.util.Random

object ProducerMain {

  def main(args: Array[String]): Unit = {
    implicit val system: ActorSystem = ActorSystem.apply("akka-stream-kafka")
    implicit val materializer: ActorMaterializer = ActorMaterializer()

    // grab our settings from the resources/application.conf file
    val producerSettings = ProducerSettings(system, new ByteArraySerializer, new StringSerializer)

    // setup a list of names to pick randomly from for our message construction
    val namesList = "Lilian" :: "Jason" :: "Arthur" :: "Omeed" :: "Eric" :: "Zach" :: "Kevin" :: Nil

    // topic to send the message to on event hubs
    val topic = "test"

    // loop until the program is exited, send message every 2 seconds
    Source.repeat(0)
      .throttle(1, FiniteDuration(2, SECONDS), 1, ThrottleMode.Shaping)
      .map(_ => {
        //construct our message here and grab a random name from our names list
        val message = s"Welcome to Azure Event Hubs ${namesList(Random.nextInt(namesList.size))}!"
        println(s"Message sent to topic - $topic - $message")
        new ProducerRecord[Array[Byte], String](topic, message.getBytes, message.toString)
      })
      .runWith(Producer.plainSink(producerSettings))
  }
} 