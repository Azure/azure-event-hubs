import akka.Done;
import akka.actor.ActorSystem;
import akka.kafka.ProducerSettings;
import akka.kafka.javadsl.Producer;
import akka.stream.ActorMaterializer;
import akka.stream.Materializer;
import akka.stream.javadsl.Source;
import akka.stream.StreamLimitReachedException;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.ByteArraySerializer;
import org.apache.kafka.common.serialization.StringSerializer;
import java.util.concurrent.CompletionStage;

abstract class AbstractProducer {
	protected final ActorSystem system = ActorSystem.create("example");

    	protected final Materializer materializer = ActorMaterializer.create(system);

    	protected final ProducerSettings<byte[], String> producerSettings = ProducerSettings
		.create(system, new ByteArraySerializer(), new StringSerializer());

    	protected final KafkaProducer<byte[], String> kafkaProducer = producerSettings.createKafkaProducer();

	protected final static int RANGE = 100;

	protected void terminateWhenDone(CompletionStage<Done> result) {
		result.exceptionally(e -> {
			if (e instanceof StreamLimitReachedException){
				System.out.println("Sent " + RANGE + " messages!");
				system.terminate();
			}
			else
				system.log().error(e, e.getMessage());
			return Done.getInstance();
		})
		.thenAccept(d -> system.terminate());
	}
}

public class AkkaTestProducer extends AbstractProducer {

    private static final String TOPIC = "test";

    public static void main(String[] args) {
        new AkkaTestProducer().demo();
    }

    public void demo() {
    	System.out.println("Sending");
        // Sends integer 1-100 to Kafka topic "test"
  	CompletionStage<Done> done = Source
		.range(1, RANGE)
  		.limit(RANGE - 1)
  		.map(n -> n.toString()).map(elem -> new ProducerRecord<byte[], String>(TOPIC, elem))
  		.runWith(Producer.plainSink(producerSettings, kafkaProducer), materializer);
  	terminateWhenDone(done);
    }
}
