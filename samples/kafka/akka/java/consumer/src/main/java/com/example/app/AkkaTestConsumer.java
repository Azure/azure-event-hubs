import akka.Done;
import akka.actor.ActorSystem;
import akka.kafka.*;
import akka.stream.ActorMaterializer;
import akka.stream.Materializer;
import akka.stream.javadsl.*;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.common.serialization.ByteArrayDeserializer;
import org.apache.kafka.common.serialization.StringDeserializer;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.atomic.AtomicLong;

abstract class AbstractConsumer {
    protected final ActorSystem system = ActorSystem.create("example");

    protected final Materializer materializer = ActorMaterializer.create(system);

    // Consumer settings
    protected ConsumerSettings<byte[], String> consumerSettings = ConsumerSettings
            .create(system, new ByteArrayDeserializer(), new StringDeserializer());

    // DB
    static class DB {
        private final AtomicLong offset = new AtomicLong();

        public CompletionStage<Done> save(ConsumerRecord<byte[], String> record) {
            System.out.println("DB.save: " + record.value());
            offset.set(record.offset());
            return CompletableFuture.completedFuture(Done.getInstance());
        }

        public CompletionStage<Long> loadOffset() {
            return CompletableFuture.completedFuture(offset.get());
        }

        public CompletionStage<Done> update(String data) {
            System.out.println(data);
            return CompletableFuture.completedFuture(Done.getInstance());
        }
    }
}


public class AkkaTestConsumer extends AbstractConsumer {

    private final static String TOPIC = "test";

    public static void main(String[] args) {
        new AkkaTestConsumer().demo();
    }
    
    //Consumes each message from TOPIC at least once
    public void demo() {
        final DB db = new DB();
        akka.kafka.javadsl.Consumer.committableSource(consumerSettings, Subscriptions.topics(TOPIC))
            .mapAsync(1, msg -> db.update(msg.record().value()).thenApply(done -> msg))
            .mapAsync(1, msg -> msg.committableOffset().commitJavadsl())
            .runWith(Sink.foreach(p -> System.out.println(p)), materializer);
    }
}
