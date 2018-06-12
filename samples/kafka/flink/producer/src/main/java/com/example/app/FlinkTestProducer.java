import org.apache.flink.api.common.functions.MapFunction;
import org.apache.flink.api.common.serialization.SimpleStringSchema;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.streaming.connectors.kafka.FlinkKafkaProducer011;     //v0.11.0.0
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.Properties;

public class FlinkTestProducer {

    private static final String TOPIC = "test";
    private static final String FILE_PATH = "src/main/resources/producer.config";

    public static void main(String... args) {
        try {
            Properties properties = new Properties();
            properties.load(new FileReader(FILE_PATH));

            final StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
            DataStream stream = createStream(env);
            FlinkKafkaProducer011<String> myProducer = new FlinkKafkaProducer011<>(
                    TOPIC,    
                    new SimpleStringSchema(),   // serialization schema
                    properties);

            stream.addSink(myProducer);
            env.execute("Testing flink print");

        } catch(FileNotFoundException e){
            System.out.println("FileNotFoundException: " + e);
        } catch (Exception e) {
            System.out.println("Failed with exception:: " + e);
        }
    }

    public static DataStream createStream(StreamExecutionEnvironment env){
        return env.generateSequence(0, 200)
            .map(new MapFunction<Long, String>() {
                @Override
                public String map(Long in) {
                    return "FLINK PRODUCE " + in;
                }
            });
    }
}
