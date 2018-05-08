import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import java.sql.Timestamp;

public class TestDataReporter implements Runnable {

    private final String TOPIC;
    private Producer<Long, String> producer;

    public TestDataReporter(final Producer<Long, String> producer, String TOPIC) {
        this.producer = producer;
        this.TOPIC = TOPIC;
    }

    @Override
    public void run() {
        for(int i = 0; i < 1000; i++) {                
                long time = System.currentTimeMillis();
                System.out.println("Test Data #" + i + " from thread " + Thread.currentThread().getId());
                final ProducerRecord<Long, String> record = new ProducerRecord<Long, String>(TOPIC, time, "Test Data #" + i);
                producer.send(record);
        }
    }
}